/* wfobj.c: rudimentary wavefront object loader
 *
 * Copyright (c) 2019 Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * This source code is licensed under the BSD Zero Clause License found in
 * the 0BSD file in the root directory of this source tree.
 */

#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define TAG "wfobj"

#define USE_MMAP

#ifdef HAVE_LIB3D
#undef USE_MMAP
#include "asset.h"
//#else
//#define STB_IMAGE_IMPLEMENTATION
//#include "stb_image.h"
#endif

#include <utils/image.h>
#include <utils/utils.h>
#include <utils/time.h>

#include "gl.h"
#include "gm.h"
#include "wfobj.h"

#define STR_LEN 128
#define MAP_SIZE (2 * 1024000) /* 2 * 256000 vertices */

#define dealloc(mem) {\
	free((void *) mem);\
	mem = NULL;\
}

#define strip_str(str) {\
	uint8_t pos = strlen(str) - 1;\
	if (str[pos] == '\n' || str[pos] == '\r')\
	str[pos] = '\0';\
}

struct context {
	const void *assets_manager;
	uint16_t objects_num;
};

struct mmap_info {
	int fd;
	size_t len;
	char *buf;
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

struct object_info {
	const char *name;
	const char *texname;

	float *vertices;
	uint32_t vertices_idx;

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

static void unmap_file(struct mmap_info *info)
{
	if (info->buf)
		munmap((void *) info->buf, info->len);

	if (info->fd >= 0)
		close(info->fd);

	info->fd = -1;
}

static uint8_t map_file(const char *path, struct mmap_info *info)
{
	struct stat st;

	info->buf = NULL;
	info->fd = -1;

	if (stat(path, &st) < 0 || S_ISDIR(st.st_mode)) {
		ee("failed to get file size | path '%s'\n", path);
		return 0;
	}

	if ((info->fd = open(path, O_RDWR)) < 0) {
		ee("failed to open file '%s'\n", path);
		return 0;
	}

	info->buf = (char *) mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE,
	  info->fd, 0);

	if (!info->buf) {
		ee("failed to map file %s\n", path);
		close(info->fd);
		info->fd = -1;
		return 0;
	}

	info->len = st.st_size;
	return 1;
}

static char *read_file(const char *path, size_t *len)
{
	struct stat st;

	if (stat(path, &st) < 0 || S_ISDIR(st.st_mode)) {
		ee("failed to get file size | path '%s'\n", path);
		return NULL;
	}

	int fd;

	if ((fd = open(path, O_RDONLY)) < 0) {
		ee("failed to open file '%s'\n", path);
		return NULL;
	}

	size_t ret;
	char *buf;

	if (!(buf = calloc(1, st.st_size))) {
		ee("failed to allocate %zu bytes\n", st.st_size);
	} else {
		if ((ret = read(fd, buf, st.st_size)) == st.st_size) {
			*len = st.st_size;
		} else {
			ee("failed to read %zu bytes from file '%s'\n",
			  st.st_size, path);
			dealloc(buf);
		}
	}

	close(fd);
	return buf;
}

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

#ifdef HAVE_LIB3D
static GLuint load_texture(const char *path, struct texture_cache *cache)
{
	uint16_t w;
	uint16_t h;
	uint32_t fmt;

	if (!path || !cache->assets_manager)
		return cache->deftex;

	return cache->deftex;
}
#else
static GLuint load_texture(const char *path, struct texture_cache *cache)
{
	if (!path)
		return cache->deftex;

	int w;
	int h;
	uint32_t fmt;
	struct list_head *cur;
	struct texture_info *texinfo;

	list_walk(cur, &cache->textures) {
		texinfo = texcache_item(cur);

		if (strcmp(path, texinfo->name) == 0)
			return texinfo->id;
	}

	int planes;
	struct image img;

	if (!readpng(path, &img))
		return cache->deftex;

	GLuint texid = generate_texture(img.data, img.w, img.h, img.format);
	free(img.data);

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
#endif

static inline void upload_object(struct wfobj *obj, struct texture_cache *cache)
{
	if (!obj->array_size || !obj->indices_num) {
		ww("bad array or indices size: %u %u\n", obj->array_size,
		  obj->indices_num);
		return;
	}

	glGenBuffers(1, &obj->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, obj->vbo);
	glBufferData(GL_ARRAY_BUFFER, obj->array_size * sizeof(float),
	  obj->array, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &obj->ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj->ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
	  obj->indices_num * sizeof(uint16_t), obj->indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

#ifdef HAVE_LIB3D
	if (obj->ctx->assets_manager && !obj->tex)
		obj->tex = load_texture(obj->texname, cache);
	else if (!obj->tex)
		obj->tex = load_texture(obj->texname, cache);
#else
	obj->tex = load_texture(obj->texname, cache);
#endif

	ii("prepared | vbo %u, %u elements | ibo %u, %u indices | tex %u\n",
	  obj->vbo, obj->array_size, obj->ibo, obj->indices_num, obj->tex);
}

static uint8_t prepare_object(struct model *model, struct object_info *info)
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

	ii("prepare object '%s' : vi %u, ni %u, ti %u | v %.1f, n %.1f, uvs %.1f\n",
	  info->name, info->vertex_indices_idx, info->normal_indices_idx,
	  info->uv_indices_idx, info->vertices_idx / 3., info->normals_idx / 3.,
	  info->uvs_idx / 2.);

	struct wfobj *obj = calloc(1, sizeof(*obj));

	if (!obj) {
		ee("failed to allocate object memory\n");
		return 0;
	}

	uint32_t prot = PROT_READ | PROT_WRITE;
	uint32_t flags = MAP_ANONYMOUS | MAP_PRIVATE;

	if (!(obj->array = mmap(NULL, MAP_SIZE, prot, flags, -1, 0))) {
		ee("failed to map array buffer for obj '%s'\n", obj->name);
		return 0;
	}

	if (!(obj->indices = mmap(NULL, MAP_SIZE, prot, flags, -1, 0))) {
		ee("failed to map indices buffer for obj '%s'\n", obj->name);
		return 0;
	}

	struct context *ctx = (struct context *) model->ctx;

	obj->id = ctx->objects_num++;
	obj->name = strdup(info->name);
	obj->visible = 1;

	if (info->texname) {
		obj->texname = strdup(info->texname);
		info->texname = NULL;
	}

	ii("obj '%s' id %u | vertex indices %u\n", obj->name, obj->id, info->vertex_indices_idx);

	for (uint32_t i = 0; i < info->vertex_indices_idx; ++i) {
		uint16_t vertex_index = info->vertex_indices[i];
		float vx = info->vertices[vertex_index * 3];
		float vy = info->vertices[vertex_index * 3 + 1];
		float vz = info->vertices[vertex_index * 3 + 2];

		obj->array[obj->array_size++] = vx;
		obj->array[obj->array_size++] = vy;
		obj->array[obj->array_size++] = vz;

		obj->indices[obj->indices_num++] = i;

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

		obj->array[obj->array_size++] = nx;
		obj->array[obj->array_size++] = ny;
		obj->array[obj->array_size++] = nz;

		float tx;
		float ty;

		if (!info->uv_indices_idx) {
			tx = ty = 0;
		} else {
			uint32_t uv_index = info->uv_indices[i];

			tx = info->uvs[uv_index * 2];
			ty = info->uvs[uv_index * 2 + 1];
		}

		obj->array[obj->array_size++] = tx;
		obj->array[obj->array_size++] = ty;

		dd("[%u] %u | v { %.4f %.4f %.4f } | n { %.4f %.4f %.4f } | t { %.4f %.4f }\n",
		  i, vertex_index, vx, vy, vz, nx, ny, nz, tx, ty);
	}

	list_add(&model->objects, &obj->head);

	info->vertices_idx = 0;
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
	struct mmap_info mapinfo;

	ii("prepare materials library %s\n", path);

#ifdef USE_MMAP
	if (!map_file(path, &mapinfo))
		return;

	prepare_texlib(mapinfo.buf, mapinfo.len, texlib);
	unmap_file(&mapinfo);
#else
	size_t len;
	char *buf = read_file(path, &len);

	if (!buf)
		return;

	prepare_texlib(buf, len, texlib);
	free(buf);
#endif /* USE_MMAP */
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

static void prepare_indices(char *str, struct object_info *info)
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
			uint32_t i = info->vertex_indices_idx - 1;
			dd("vertex_indices[%u] %u | next str '%s'\n", i, info->vertex_indices[i], ptr);
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

			info->uv_indices[info->uv_indices_idx++] = ti[0];
			info->uv_indices[info->uv_indices_idx++] = ti[1];
			info->uv_indices[info->uv_indices_idx++] = ti[2];
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

			info->uv_indices[info->uv_indices_idx++] = ti[0];
			info->uv_indices[info->uv_indices_idx++] = ti[1];
			info->uv_indices[info->uv_indices_idx++] = ti[2];
			info->uv_indices[info->uv_indices_idx++] = ti[0];
			info->uv_indices[info->uv_indices_idx++] = ti[2];
			info->uv_indices[info->uv_indices_idx++] = ti[3];
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

static inline void unmap_object_info(struct object_info *info)
{
	munmap(info->vertices, MAP_SIZE);
	info->vertices = NULL;
	munmap(info->vertex_indices, MAP_SIZE);
	info->vertex_indices = NULL;

	munmap(info->normals, MAP_SIZE);
	info->normals = NULL;
	munmap(info->normal_indices, MAP_SIZE);
	info->normal_indices = NULL;

	munmap(info->uvs, MAP_SIZE);
	info->uvs = NULL;
	munmap(info->uv_indices, MAP_SIZE);
	info->uv_indices = NULL;
}

static inline uint8_t map_object_info(struct object_info *info)
{
	uint32_t prot = PROT_READ | PROT_WRITE;
	uint32_t flags = MAP_ANONYMOUS | MAP_PRIVATE;

	info->vertices_idx = 0;
	info->vertex_indices_idx = 0;

	info->normals_idx = 0;
	info->normal_indices_idx = 0;

	info->uvs_idx = 0;
	info->uv_indices_idx = 0;

	if (!(info->vertices = mmap(NULL, MAP_SIZE, prot, flags, -1, 0)))
		return 0;

	if (!(info->vertex_indices = mmap(NULL, MAP_SIZE, prot, flags, -1, 0)))
		return 0;

	if (!(info->normals = mmap(NULL, MAP_SIZE, prot, flags, -1, 0)))
		return 0;

	if (!(info->normal_indices = mmap(NULL, MAP_SIZE, prot, flags, -1, 0)))
		return 0;

	if (!(info->uvs = mmap(NULL, MAP_SIZE, prot, flags, -1, 0)))
		return 0;

	if (!(info->uv_indices = mmap(NULL, MAP_SIZE, prot, flags, -1, 0)))
		return 0;

	return 1;
}

uint8_t load_model(char *buf, size_t len, struct model *model)
{
	struct list_head *cur;
	struct texlib texlib;
	struct object_info info;
	uint8_t ret = 0;
	char *ptr = buf;
	char *end = buf + len;
	char *str = NULL;
	uint8_t color_idx = 0;
	uint32_t start_time = time_ms();

	list_init(&texlib.items);

	if (!map_object_info(&info)) {
		ee("failed to map object info data\n");
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
				prepare_object(model, &info);

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
		} else if (str[0] == 'v' && str[1] == ' ') { /* vertex */
			union gm_point3 vertex;
			uint8_t n = sscanf(str, "v %f %f %f\n", &vertex.x, &vertex.y, &vertex.z);

			if (n != 3) {
				ee("'v float float float' is expected | str: '%s'\n", str);
				goto out;
			}

			info.vertices[info.vertices_idx++] = vertex.x;
			info.vertices[info.vertices_idx++] = vertex.y;
			info.vertices[info.vertices_idx++] = vertex.z;
		} else if (str[0] == 'v' && str[1] == 'n') { /* vertex normal */
			union gm_point3 normal;
			uint8_t n = sscanf(str, "vn %f %f %f\n", &normal.x, &normal.y, &normal.z);

			if (n != 3) {
				ee("'vn float float float' is expected | str: '%s'\n", str);
				goto out;
			}

			info.normals[info.normals_idx++] = normal.x;
			info.normals[info.normals_idx++] = normal.y;
			info.normals[info.normals_idx++] = normal.z;
		} else if (str[0] == 'v' && str[1] == 't') { /* texture uv */
			union gm_point2 uv;
			uint8_t n = sscanf(str, "vt %f %f\n", &uv.x, &uv.y);

			if (n != 2) {
				ee("'vt float float' is expected | str: '%s'\n", str);
				goto out;
			}

			info.uvs[info.uvs_idx++] = uv.x;
			info.uvs[info.uvs_idx++] = uv.y;
		}

		str = NULL;
	}

	if (info.name)
		ret = prepare_object(model, &info);

	ii("prepared %u objects in %u ms\n", info.ctx->objects_num,
	  time_ms() - start_time);

out:
	free((void *) info.name);
	unmap_object_info(&info);

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

	list_walk_safe(cur, tmp, &model->objects) {
		struct wfobj *obj = container_of(cur, struct wfobj, head);

		glDeleteBuffers(1, &obj->vbo);
		glDeleteBuffers(1, &obj->ibo);
		glDeleteTextures(1, &obj->tex);

		list_del(&obj->head);

		free(obj->name);
		free(obj->texname);
		free(obj);
	}

	dealloc(model->ctx);
}

uint8_t prepare_model(const char *path, struct model *model, void *assets_manager)
{
	ii("read file %s\n", path);

	if (!prepare_context(model)) {
		ee("failed to create model context\n");
		return 0;
	}

	list_init(&model->objects);

	((struct context *) model->ctx)->assets_manager = assets_manager;
	struct mmap_info mapinfo;

#ifdef USE_MMAP
	if (!map_file(path, &mapinfo)) {
		dealloc(model->ctx);
		return 0;
	}

	uint8_t ret = load_model(mapinfo.buf, mapinfo.len, model);

	if (!ret)
		erase_model(model);

	unmap_file(&mapinfo);
	return ret;
#else
	size_t len;
	char *buf = read_file(path, &len);

	if (!buf) {
		dealloc(model->ctx);
		return 0;
	}

	uint8_t ret = load_model(buf, len, model);

	if (!ret)
		erase_model(model);

	return ret;
#endif
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

	list_walk(cur, &model->objects) {
		struct wfobj *obj = container_of(cur, struct wfobj, head);

		obj->tex = cache.deftex;
		upload_object(obj, &cache);

		ii("upload obj %u %s | %u elements, %u indices | tex %u %s | visible %u\n",
		  obj->id, obj->name, obj->array_size, obj->indices_num,
		  obj->tex, obj->texname, obj->visible);

		array_bytes += obj->array_size;
		indices_bytes += obj->indices_num;

		dealloc(obj->name);
		dealloc(obj->texname);

		munmap(obj->indices, MAP_SIZE);
		obj->indices = NULL;
		munmap(obj->array, MAP_SIZE);
		obj->array = NULL;
	}

	struct list_head *tmp;

	list_walk_safe(cur, tmp, &cache.textures) {
		struct texture_info *texinfo = texcache_item(cur);

		list_del(&texinfo->head);
		free(texinfo);
	}

	ii("uploaded %u objects in %u ms | total bytes: array %u indices %u\n",
	  cache.ctx->objects_num, time_ms() - start_time,
	  (uint32_t) (array_bytes * sizeof(float)),
	  (uint32_t) (indices_bytes * sizeof(uint16_t)));
}
