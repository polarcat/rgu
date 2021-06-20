/* wfobj.c: rudimentary wavefront object loader
 *
 * Copyright (c) 2019 Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * This source code is licensed under the BSD Zero Clause License found in
 * the 0BSD file in the root directory of this source tree.
 */

#include <stdlib.h>
#include <libgen.h>

#define TAG "wfobj"

#include <rgu/mem.h>
#include <rgu/asset.h>
#include <rgu/image.h>
#include <rgu/utils.h>
#include <rgu/time.h>
#include <rgu/gl.h>
#include <rgu/gm.h>
#include <rgu/wfobj.h>

#define MAX_INDEX 100000

#define strip_str(str) {\
	uint8_t pos = strlen(str) - 1;\
	if (str[pos] == '\n' || str[pos] == '\r')\
	str[pos] = '\0';\
}

struct context {
	const void *amgr;
	uint16_t shapes_num;
};

struct texlib_item {
	char *material;
	char *texname;
	struct list_head head;
};

struct texlib {
	struct list_head items;
	uint16_t items_num;
	struct context *ctx;
};

#define texlib_item(item) container_of(item, struct texlib_item, head)

struct texture_info {
	GLuint id;
	const char *name;
	struct list_head head;
};

struct texture_cache {
	GLuint deftex;
	struct list_head textures;
	struct context *ctx;
};

#define texcache_item(item) container_of(item, struct texture_info, head)

struct shape_info {
	const char *name;
	const char *texname;

	float *vertices;
	uint32_t vertices_idx;

	float *colors;
	uint32_t colors_idx;

	uint16_t *vertex_indices;
	uint32_t vertex_indices_idx;

	float *normals;
	uint32_t normals_idx;

	uint16_t *normal_indices;
	uint32_t normal_indices_idx;

	float *uvs;
	uint32_t uvs_idx;

	uint16_t *uv_indices;
	uint32_t uv_indices_idx;

	struct context *ctx;
};

static GLuint default_texture(void)
{
	GLuint tex;
	uint32_t fmt = GL_RGB;
	uint8_t rgb[3] = { 0xff, 0xff, 0xff };

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, fmt, 1, 1, 0, fmt, GL_UNSIGNED_BYTE, rgb);
	glBindTexture(GL_TEXTURE_2D, 0);

	ii("default texture %u | color { %u %u %u }\n", tex, rgb[0], rgb[1], rgb[2]);

	return tex;
}

static GLuint generate_texture(void *data, uint16_t w, uint16_t h, uint32_t fmt)
{
	GLuint tex;

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);

	dd("texture %u fmt %u wh { %u %u } data %p\n", tex, fmt, w, h, data);

	return tex;
}

static GLuint load_texture(const char *path, struct texture_cache *cache)
{
	if (!path)
		return cache->deftex;

	struct list_head *cur;
	struct texture_info *texinfo;

	list_walk(cur, &cache->textures) {
		texinfo = texcache_item(cur);

		if (strcmp(path, texinfo->name) == 0)
			return texinfo->id;
	}

	struct asset_info ainfo;
	struct image_info iinfo;

	if (!get_asset(path, &ainfo, cache->ctx->amgr))
		return cache->deftex;

	if (!get_image(&ainfo, &iinfo)) {
		put_asset(&ainfo);
		return cache->deftex;
	}

	uint32_t fmt;

	if (iinfo.planes == 3)
		fmt = GL_RGB;
	else if (iinfo.planes == 4)
		fmt = GL_RGBA;
	else
		fmt = GL_RGB;

	GLuint texid = generate_texture(iinfo.image, iinfo.w, iinfo.h, fmt);

	put_image(&iinfo);
	put_asset(&ainfo);

	if (!(texinfo = calloc(1, sizeof(*texinfo)))) {
		ee("failed to allocate memory for texture cache item\n");
		glDeleteTextures(1, &texid);
		return cache->deftex;
	}

	texinfo->name = path;
	texinfo->id = texid;
	list_add(&cache->textures, &texinfo->head);

	ii("use texture %u %s\n", texid, path);

	return texid;
}

static inline void upload_shape(struct wfobj *shape,
  struct texture_cache *cache)
{
	if (!shape->array_size || !shape->indices_num) {
		ww("bad array or indices size: %u %u\n", shape->array_size,
		  shape->indices_num);
		return;
	}

	glGenBuffers(1, &shape->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, shape->vbo);
	glBufferData(GL_ARRAY_BUFFER, shape->array_size * sizeof(float),
	  shape->array, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &shape->ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shape->ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
	  shape->indices_num * sizeof(uint16_t), shape->indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	shape->tex = load_texture(shape->texname, cache);

	ii("shape uploaded | vbo %u, %u elements | ibo %u, %u indices | tex %u | with color %u\n",
	  shape->vbo, shape->array_size, shape->ibo, shape->indices_num,
	  shape->tex, shape->with_color);
}

static uint8_t prepare_shape(struct model *model, struct shape_info *info)
{
	if (info->normal_indices_idx &&
	  (info->normal_indices_idx != info->vertex_indices_idx)) {
		ee("normal and vertex indices mismatch | n %u != v %u\n",
		  info->normal_indices_idx, info->vertex_indices_idx);
		return 0;
	}

	if (info->uv_indices_idx &&
	  (info->uv_indices_idx != info->vertex_indices_idx)) {
		ee("uv and vertex indices mismatch | uv %u != v %u\n",
		  info->uv_indices_idx, info->vertex_indices_idx);
		return 0;
	}

	ii("prepare shape '%s' : vi %u, ni %u, ti %u | v %.1f, n %.1f, uvs %.1f\n",
	  info->name, info->vertex_indices_idx, info->normal_indices_idx,
	  info->uv_indices_idx, info->vertices_idx / 3., info->normals_idx / 3.,
	  info->uvs_idx / 2.);

	struct wfobj *shape = calloc(1, sizeof(*shape));

	if (!shape) {
		ee("failed to allocate shape memory\n");
		return 0;
	}

	uint32_t size;

	size = info->vertex_indices_idx * sizeof(*info->vertices) * 3;
	size += info->normal_indices_idx * sizeof(*info->normals) * 3;
	size += info->uv_indices_idx * sizeof(*info->uvs) * 2;

	if (info->colors_idx)
		size += info->vertex_indices_idx * sizeof(*info->colors) * 3;

	if (!(shape->array = malloc(size))) {
		ee("failed to allocate %u bytes for shape '%s'\n",
		  size, shape->name);
		return 0;
	}

	size = info->vertex_indices_idx * sizeof(*shape->indices);

	if (!(shape->indices = malloc(size))) {
		ee("failed to allocate %u bytes for shape '%s'\n",
		  size, shape->name);
		dealloc(shape->array);
		return 0;
	}

	struct context *ctx = (struct context *) model->ctx;

	shape->id = ctx->shapes_num++;
	shape->name = strdup(info->name);
	shape->visible = 1;
	shape->with_color = !!info->colors_idx;

	if (info->texname) {
		shape->texname = strdup(info->texname);
		info->texname = NULL;
	}

	ii("shape '%s' id %u | vertex indices %u\n", shape->name, shape->id,
	  info->vertex_indices_idx);

	for (uint32_t i = 0; i < info->vertex_indices_idx; ++i) {
		uint16_t vertex_index = info->vertex_indices[i];
		float vx = info->vertices[vertex_index * 3];
		float vy = info->vertices[vertex_index * 3 + 1];
		float vz = info->vertices[vertex_index * 3 + 2];

		shape->array[shape->array_size++] = vx;
		shape->array[shape->array_size++] = vy;
		shape->array[shape->array_size++] = vz;

		if (model->max.x < vx)
			model->max.x = vx;
		if (model->max.y < vy)
			model->max.y = vy;
		if (model->max.z < vz)
			model->max.z = vz;

		if (model->min.x > vx)
			model->min.x = vx;
		if (model->min.y > vy)
			model->min.y = vy;
		if (model->min.z > vz)
			model->min.z = vz;

		shape->indices[shape->indices_num++] = i;

		float nx;
		float ny;
		float nz;

		if (!info->normal_indices_idx) {
			nx = ny = nz = 0;
		} else {
			uint32_t normal_index = info->normal_indices[i];

			nx = info->normals[normal_index * 3];
			ny = info->normals[normal_index * 3 + 1];
			nz = info->normals[normal_index * 3 + 2];
		}

		shape->array[shape->array_size++] = nx;
		shape->array[shape->array_size++] = ny;
		shape->array[shape->array_size++] = nz;

		float tx;
		float ty;

		if (!info->uv_indices_idx) {
			tx = ty = 0;
		} else {
			uint32_t uv_index = info->uv_indices[i];

			tx = info->uvs[uv_index * 2];
			ty = info->uvs[uv_index * 2 + 1];
		}

		shape->array[shape->array_size++] = tx;
		shape->array[shape->array_size++] = ty;

		if (shape->with_color) {
			//vertex_index *= 3;;
			shape->color.r = info->colors[vertex_index * 3];
			shape->color.g = info->colors[vertex_index * 3 + 1];
			shape->color.b = info->colors[vertex_index * 3 + 2];

			shape->array[shape->array_size++] = shape->color.r;
			shape->array[shape->array_size++] = shape->color.g;
			shape->array[shape->array_size++] = shape->color.b;
			dd("vertex %u | color (%f %f %f)\n", vertex_index,
			  shape->color.r, shape->color.g, shape->color.b);
		}

		dd("[%u] %u | v { %.4f %.4f %.4f } | n { %.4f %.4f %.4f } | t { %.4f %.4f }\n",
		  i, vertex_index, vx, vy, vz, nx, ny, nz, tx, ty);
	}

	list_add(&model->shapes, &shape->head);

	info->vertices_idx = 0;
	info->colors_idx = 0;
	info->vertex_indices_idx = 0;

	info->normals_idx = 0;
	info->normal_indices_idx = 0;

	info->uvs_idx = 0;
	info->uv_indices_idx = 0;

	return 1;
}

static void prepare_texlib(char *buf, size_t len, struct texlib *texlib)
{
	struct list_head *cur;
	char *mtl = NULL;
	char *ptr = buf;
	char *end = buf + len;
	char *str = NULL;
	uint16_t texnum = 0;

	while (*ptr != '\0' && ptr < end) {
		if (!str)
			str = ptr;

		if (*ptr++ != '\n') /* BEWARE of '\r' endings! */
			continue;

		*(ptr - 1) = '\0';

		dd("str: '%s'\n", str);

		if (str[0] == 'n' && str[1] == 'e' && str[2] == 'w' &&
		  str[3] == 'm' && str[4] == 't' && str[5] == 'l' &&
		  str[6] == ' ') {
			strip_str(str);

			uint8_t found = 0;

			list_walk(cur, &texlib->items) {
				struct texlib_item *item = texlib_item(cur);

				if (strcmp(&str[7], item->material) == 0) {
					found = 1;
					break;
				}
			}

			if (!found)
				mtl = strdup(&str[7]);
		} else if (str[0] == 'm' && str[1] == 'a' && str[2] == 'p' &&
				str[3] == '_' && str[4] == 'K' && str[5] == 'd' &&
				str[6] == ' ') {
			if (!mtl)
				continue;

			strip_str(str);
			struct texlib_item *item = calloc(1, sizeof(*item));

			if (!item) {
				free(mtl);
			} else {
				item->material = mtl;
				item->texname = strdup(&str[7]);
				list_add(&texlib->items, &item->head);
				texnum++;
			}

			mtl = NULL;
		}

		str = NULL;
	}

	ii("found %u textures\n", texnum);

	list_walk(cur, &texlib->items) {
		struct texlib_item *item = texlib_item(cur);

		ii("material %s | texture %s\n", item->material, item->texname);
	}
}

static void prepare_mtllib(const char *path, struct texlib *texlib)
{
	struct asset_info ainfo;

	ii("prepare materials library %s\n", path);

	if (!get_asset(path, &ainfo, texlib->ctx->amgr))
		return;

	prepare_texlib((char *) ainfo.buf, ainfo.len, texlib);
	put_asset(&ainfo);
}

static inline char *parse_indices(char *token, uint16_t *vi, uint16_t *ti,
  uint16_t *ni)
{
	dd("token %#x | str: %s\n", *token & 0xff, token);

	if (!token || *token == '\0')
		return NULL;

	char *ptr = token;
	char *end = ptr + strlen(token);

	*vi = *ti = *ni = 1;

	while (ptr < end) {
		if ((*vi = atoi(ptr)))
			*vi -= 1;

		if (!(ptr = strchr(ptr, '/')))
			break; /* only vertex indices */

		ptr++;

		if (*ptr == '/') {
			ptr++; /* no uv indices */
		} else {
			if ((*ti = atoi(ptr)))
				*ti -= 1;

			if (!(ptr = strchr(ptr, '/'))) {
				ptr = end;
				break; /* no normal indices */
			}

			ptr++;
		}

		if (ptr < end)
			if ((*ni = atoi(ptr)))
				*ni -= 1;

		ptr += strlen(ptr);

		if (*vi && *ti && *ni)
			break;
	}

	ptr++; /* skip '\0' */

	dd("--> %u %u %u | ptr: %s\n", *vi, *ti, *ni, ptr);
	return ptr;
}

static void prepare_indices(char *str, struct shape_info *info)
{
	char *ptr = str;
	char *end = ptr + strlen(ptr);

	uint16_t vi[4];
	uint16_t ti[4];
	uint16_t ni[4];

	dd("prepare indices from str: %s\n", ptr);

	/* split string */

	uint8_t multi = 0;
	uint8_t verts_num = 1;

	while (ptr++ < end) {
		if (*ptr == ' ') {
			*ptr = '\0';
			verts_num++;
		} else if (*ptr == '/') {
			multi = 1;
		}
	}

	dd("multi %u | %u vertices | str: '%s'\n", multi, verts_num, str);
	ptr = str;

	uint8_t n = 0;

	while (ptr && ptr < end) {
		if (multi) {
			ptr = parse_indices(ptr, &vi[n], &ti[n], &ni[n]);
			dd("%u | vi %u ti %u ni %u\n", n, vi[n], ti[n], ni[n]);
		} else { /* vertex indices only */
			info->vertex_indices[info->vertex_indices_idx++] = atoi(ptr);
			ptr += strlen(ptr) + 1;
			dd("vertex_indices[%u] %u | next str '%s'\n",
			  info->vertex_indices_idx - 1,
			  info->vertex_indices[i], ptr);
			continue;
		}

		n++;

		if (verts_num == 3 && n == 3) {
			n = 0;
			info->vertex_indices[info->vertex_indices_idx++] = vi[0];
			info->vertex_indices[info->vertex_indices_idx++] = vi[1];
			info->vertex_indices[info->vertex_indices_idx++] = vi[2];

			info->normal_indices[info->normal_indices_idx++] = ni[0];
			info->normal_indices[info->normal_indices_idx++] = ni[1];
			info->normal_indices[info->normal_indices_idx++] = ni[2];

			if (info->uv_indices) {
				info->uv_indices[info->uv_indices_idx++] = ti[0];
				info->uv_indices[info->uv_indices_idx++] = ti[1];
				info->uv_indices[info->uv_indices_idx++] = ti[2];
			}
		} else if (verts_num == 4 && n == 4) { /* then triangulate */
			n = 0;
			info->vertex_indices[info->vertex_indices_idx++] = vi[0];
			info->vertex_indices[info->vertex_indices_idx++] = vi[1];
			info->vertex_indices[info->vertex_indices_idx++] = vi[2];
			info->vertex_indices[info->vertex_indices_idx++] = vi[0];
			info->vertex_indices[info->vertex_indices_idx++] = vi[2];
			info->vertex_indices[info->vertex_indices_idx++] = vi[3];

			info->normal_indices[info->normal_indices_idx++] = ni[0];
			info->normal_indices[info->normal_indices_idx++] = ni[1];
			info->normal_indices[info->normal_indices_idx++] = ni[2];
			info->normal_indices[info->normal_indices_idx++] = ni[0];
			info->normal_indices[info->normal_indices_idx++] = ni[2];
			info->normal_indices[info->normal_indices_idx++] = ni[3];

			if (info->uv_indices) {
				info->uv_indices[info->uv_indices_idx++] = ti[0];
				info->uv_indices[info->uv_indices_idx++] = ti[1];
				info->uv_indices[info->uv_indices_idx++] = ti[2];
				info->uv_indices[info->uv_indices_idx++] = ti[0];
				info->uv_indices[info->uv_indices_idx++] = ti[2];
				info->uv_indices[info->uv_indices_idx++] = ti[3];
			}
		} else if (n > 4) {
			ww("more than 4 vertices per face is not supported\n");
			n = 0;
		}
	}

#if 0
	printf("%u indices: ", info->vertex_indices_idx);

	for (uint8_t i = 0; i < info->vertex_indices_idx; ++i)
		printf("%u ", info->vertex_indices[i] - 1);

	printf("\n\n");
#endif
}

static uint8_t prepare_context(struct model *model)
{
	if (model->ctx) {
		return 1;
	} else if (!(model->ctx = calloc(1, sizeof(struct context)))) {
		ee("failed to create model context\n");
		return 0;
	}

	return 1;
}

static inline void free_shape_data(struct shape_info *info)
{
	dealloc(info->vertices);
	dealloc(info->colors);
	dealloc(info->vertex_indices);
	dealloc(info->normals);
	dealloc(info->normal_indices);
	dealloc(info->uvs);
	dealloc(info->uv_indices);
}

#define alloc_component(component) malloc(sizeof(*(component)) * MAX_INDEX)

static inline uint8_t alloc_shape_data(struct shape_info *info,
  uint8_t ignore_texture)
{
	info->vertices_idx = 0;
	info->colors_idx = 0;
	info->vertex_indices_idx = 0;

	info->normals_idx = 0;
	info->normal_indices_idx = 0;

	info->uvs_idx = 0;
	info->uv_indices_idx = 0;

	if (!(info->vertices = alloc_component(info->vertices)))
		goto err;

	if (!(info->colors = alloc_component(info->colors)))
		goto err;

	if (!(info->vertex_indices = alloc_component(info->vertex_indices)))
		goto err;

	if (!(info->normals = alloc_component(info->normals)))
		goto err;

	if (!(info->normal_indices = alloc_component(info->normal_indices)))
		goto err;


	if (ignore_texture) {
		info->uvs = NULL;
		info->uv_indices = NULL;
	} else {
		if (!(info->uvs = alloc_component(info->uvs)))
			goto err;

		if (!(info->uv_indices = alloc_component(info->uv_indices)))
			goto err;
	}

	return 1;

err:
	free_shape_data(info);
	return 0;
}

static uint8_t prepare_vertices(char *str, struct shape_info *info)
{
	union gm_point3 vertex;
	union color_rgb color;
	uint8_t n;

	/* check if color information is attached */
	n = sscanf(str, "v %f %f %f %f %f %f\n",
	  &vertex.x, &vertex.y, &vertex.z, &color.r, &color.g, &color.b);

	if (n == 6) {
		info->vertices[info->vertices_idx++] = vertex.x;

		if (info->vertices_idx >= MAX_INDEX) {
			ee("num of vertices is out of range %u\n", MAX_INDEX);
			return 0;
		}

		info->vertices[info->vertices_idx++] = vertex.y;

		if (info->vertices_idx >= MAX_INDEX) {
			ee("num of vertices is out of range %u\n", MAX_INDEX);
			return 0;
		}

		info->vertices[info->vertices_idx++] = vertex.z;

		if (info->vertices_idx >= MAX_INDEX) {
			ee("num of vertices is out of range %u\n", MAX_INDEX);
			return 0;
		}

		info->colors[info->colors_idx++] = color.r;
		info->colors[info->colors_idx++] = color.g;
		info->colors[info->colors_idx++] = color.b;
		return 1;
	}

	n = sscanf(str, "v %f %f %f\n", &vertex.x, &vertex.y, &vertex.z);

	if (n != 3) {
		ee("'v float float float' is expected | str: '%s'\n", str);
		return 0;
	}

	info->vertices[info->vertices_idx++] = vertex.x;
	info->vertices[info->vertices_idx++] = vertex.y;
	info->vertices[info->vertices_idx++] = vertex.z;

	return 1;
}

static uint8_t prepare_normals(char *str, struct shape_info *info)
{
	union gm_point3 normal;
	uint8_t n;

	n = sscanf(str, "vn %f %f %f\n", &normal.x, &normal.y, &normal.z);

	if (n != 3) {
		ee("'vn float float float' is expected | str: '%s'\n", str);
		return 0;
	}

	info->normals[info->normals_idx++] = normal.x;
	info->normals[info->normals_idx++] = normal.y;
	info->normals[info->normals_idx++] = normal.z;

	return 1;
}

static uint8_t prepare_uv(char *str, struct shape_info *info)
{
	union gm_point2 uv;
	uint8_t n = sscanf(str, "vt %f %f\n", &uv.x, &uv.y);

	if (n != 2) {
		ee("'vt float float' is expected | str: '%s'\n", str);
		return 0;
	}

	info->uvs[info->uvs_idx++] = uv.x;
	info->uvs[info->uvs_idx++] = uv.y;

	return 1;
}

uint8_t load_model(char *buf, size_t len, struct model *model)
{
	struct list_head *cur;
	struct texlib texlib;
	struct shape_info info;
	uint8_t ret = 0;
	char *ptr = buf;
	char *end = buf + len;
	char *str = NULL;
	uint32_t start_time = time_ms();

	list_init(&texlib.items);

	if (!alloc_shape_data(&info, model->ignore_texture)) {
		ee("failed to map shape info data\n");
		return 0;
	}

	ii("loading model from buf %p len %zu\n", buf, len);

	info.ctx = (struct context *) model->ctx;
	info.name = NULL;
	info.texname = NULL;

	while (*ptr != '\0' && ptr < end) {
		if (!str)
			str = ptr;

		if (*ptr++ != '\n') /* BEWARE of '\r' endings! */
			continue;

		*(ptr - 1) = '\0';

		dd("str: '%s'\n", str);

		if ((str[0] == 'o' || str[0] == 'g') && str[1] == ' ') {
			dd("o or g: %s\n", str);

			if (info.vertex_indices_idx)
				prepare_shape(model, &info);

			strip_str(str);
			info.name = strdup(&str[2]);
		} else if (model->rgb.r < 0 &&
		  str[0] == 'm' && str[1] == 't' && str[2] == 'l' &&
		  str[3] == 'l' && str[4] == 'i' && str[5] == 'b' &&
		  str[6] == ' ') {
			strip_str(str);
			prepare_mtllib(&str[7], &texlib);
		} else if (model->rgb.r < 0 &&
		  str[0] == 'u' && str[1] == 's' && str[2] == 'e' &&
		  str[3] == 'm' && str[4] == 't' && str[5] == 'l' &&
		  str[6] == ' ') {
			strip_str(str);

			ii("use mtl: %s\n", str);

			list_walk(cur, &texlib.items) {
				struct texlib_item *item = texlib_item(cur);

				if (strcmp(&str[7], item->material) == 0) {
					info.texname = item->texname;
					break;
				}
			}
		} else if (str[0] == 'f' && str[1] == ' ') {
			prepare_indices(str + 2, &info); /* f[[:space:]] */
		} else if (str[0] == 'v' && str[1] == ' ') {
			if (!prepare_vertices(str, &info))
				goto out;
		} else if (str[0] == 'v' && str[1] == 'n') {
			if (!prepare_normals(str, &info))
				goto out;
		} else if (!model->ignore_texture &&
		  str[0] == 'v' && str[1] == 't') { /* texture uv */
			if (!prepare_uv(str, &info))
				goto out;
		}

		str = NULL;
	}

	if (!info.name)
		info.name = strdup(model->name);

	ret = prepare_shape(model, &info);

	ii("prepared %u shapes in %u ms\n", info.ctx->shapes_num,
	  (uint32_t) time_ms() - start_time);

	ii("model extents min { %.4f %.4f %.4f } max { %.4f %.4f %.4f }\n",
	  model->min.x, model->min.y, model->min.z,
	  model->max.x, model->max.y, model->max.z);
out:
	free((void *) info.name);
	free_shape_data(&info);

	struct list_head *tmp;

	list_walk_safe(cur, tmp, &texlib.items) {
		struct texlib_item *item = texlib_item(cur);

		list_del(&item->head);

		free(item->material);
		free(item->texname);
		free(item);
	}

	return ret;
}

void erase_model(struct model *model)
{
	struct list_head *cur;
	struct list_head *tmp;

	list_walk_safe(cur, tmp, &model->shapes) {
		struct wfobj *shape = container_of(cur, struct wfobj, head);

		glDeleteBuffers(1, &shape->vbo);
		glDeleteBuffers(1, &shape->ibo);
		glDeleteTextures(1, &shape->tex);

		list_del(&shape->head);

		free(shape->name);
		free(shape->texname);
		free(shape);
	}

	dealloc(model->ctx);
}

uint8_t prepare_model(char *path, struct model *model, void *amgr)
{
	ii("read file %s\n", path);

	if (!prepare_context(model)) {
		ee("failed to create model context\n");
		return 0;
	}

	model->min.x = model->min.y = model->min.z = UINT32_MAX;
	model->max.x = model->max.y = model->max.z = 0;

	list_init(&model->shapes);

	((struct context *) model->ctx)->amgr = amgr;

	struct asset_info ainfo;

	if (!get_asset(path, &ainfo, amgr)) {
		dealloc(model->ctx);
		return 0;
	}

	/* basename can modify path; hence call it here */
	model->name = basename(path);

	uint8_t ret = load_model((char *) ainfo.buf, ainfo.len, model);

	if (!ret)
		erase_model(model);

	put_asset(&ainfo);

	return ret;
}

void upload_model(struct model *model)
{
	struct list_head *cur;
	struct texture_cache cache;
	uint32_t start_time = time_ms();
	uint32_t array_bytes = 0;
	uint32_t indices_bytes = 0;

	list_init(&cache.textures);

	cache.ctx = (struct context *) model->ctx;
	cache.deftex = default_texture();

	list_walk(cur, &model->shapes) {
		struct wfobj *shape = container_of(cur, struct wfobj, head);

		shape->tex = cache.deftex;
		upload_shape(shape, &cache);

		ii("upload obj %u %s | %u elements, %u indices | tex %u %s | visible %u\n",
		  shape->id, shape->name, shape->array_size, shape->indices_num,
		  shape->tex, shape->texname, shape->visible);

		array_bytes += shape->array_size;
		indices_bytes += shape->indices_num;

		dealloc(shape->name);
		dealloc(shape->texname);
		dealloc(shape->indices);
		dealloc(shape->array);
	}

	struct list_head *tmp;

	list_walk_safe(cur, tmp, &cache.textures) {
		struct texture_info *texinfo = texcache_item(cur);

		list_del(&texinfo->head);
		free(texinfo);
	}

	ii("uploaded %u shapes in %u ms | total bytes: array %u indices %u\n",
	  cache.ctx->shapes_num, (uint32_t) time_ms() - start_time,
	  (uint32_t) (array_bytes * sizeof(float)),
	  (uint32_t) (indices_bytes * sizeof(uint16_t)));
}
