#ifndef __MIAL_H__
#define __MIAL_H__

#include <time.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

/* error codes */
enum mial_error {
  MIAL_ERROR_OK = 0,
  MIAL_ERROR_NULL,
  MIAL_ERROR_FULL,
  MIAL_ERROR_ALLOC,
  MIAL_ERROR_EMPTY,
  MIAL_ERROR_INDEX,
  MIAL_ERROR_AMOUNT,
  MIAL_ERROR_EXISTS,
  MIAL_ERROR_CORRUPTED,
  MIAL_ERROR_DONT_EXISTS,
  MIAL_ERROR_VERTEX_SHADER,
  MIAL_ERROR_FRAGMENT_SHADER,
  MIAL_ERROR_SHADER_TYPE,
  MIAL_ERROR_SHADER_CREATION,
  MIAL_ERROR_SHADER_COMPILATION,
  MIAL_ERROR_SHADER_LINKING,
  MIAL_ERROR_KEY,
  MIAL_ERROR_UNREACHABLE,
};

static inline const char *
mial_error_string(enum mial_error err) {
  switch (err) {
  case MIAL_ERROR_OK:                 return "Ok";
  case MIAL_ERROR_NULL:               return "Null pointer";
  case MIAL_ERROR_FULL:               return "Full";
  case MIAL_ERROR_ALLOC:              return "Allocation error";
  case MIAL_ERROR_EMPTY:              return "Empty";
  case MIAL_ERROR_INDEX:              return "Invalid index";
  case MIAL_ERROR_AMOUNT:             return "Invalid amount";
  case MIAL_ERROR_EXISTS:             return "Already exists";
  case MIAL_ERROR_CORRUPTED:          return "Corrupted data";
  case MIAL_ERROR_DONT_EXISTS:        return "Don't exists";
  case MIAL_ERROR_VERTEX_SHADER:      return "Missing or unclosed vertex shader block";
  case MIAL_ERROR_FRAGMENT_SHADER:    return "Missing or unclosed fragment shader block";
  case MIAL_ERROR_SHADER_TYPE:        return "Invalid shader type";
  case MIAL_ERROR_SHADER_CREATION:    return "Couldn't create shader";
  case MIAL_ERROR_SHADER_COMPILATION: return "Couldn't compile shader";
  case MIAL_ERROR_SHADER_LINKING:     return "Couldn't link shaders";
  case MIAL_ERROR_KEY:                return "Invalid key";
  case MIAL_ERROR_UNREACHABLE:        return "Unreachable";
  }
  return "Unknown error";
}
struct mial_u32  { uint32_t val; enum mial_error err; };
struct mial_i32  { int32_t  val; enum mial_error err; };
struct mial_ptr  { void    *val; enum mial_error err; };
struct mial_bool { bool     val; enum mial_error err; };

/* helper functions */
uint32_t mial_strlen(const char *s, uint32_t max);

/* custom array. probably not gonna use it, but still kinda of a cool trick */
#define mial_array_make(T, ...) \
((struct { \
  uint64_t size; \
  T data[sizeof (T []) { __VA_ARGS__ } / sizeof (T)]; \
}) { \
  sizeof (T []) { __VA_ARGS__ } / sizeof (T), \
  { __VA_ARGS__ } \
}.data)
#define mial_array_make_n(T, n) \
((struct { \
  uint64_t size; \
  T data[n]; \
}) { n, { 0 } }.data)
#define mial_array_size(array) (*(((const uint64_t *)array) - 1))

/* list definitions */
struct mial_ptr __mial_list_make__(uint32_t element_size, uint32_t grow_by);
struct mial_ptr mial_list_copy(const void *src);
enum mial_error __mial_list_reserve__(void **plist, uint32_t amount);
enum mial_error __mial_list_resize__(void **plist, uint32_t amount);
enum mial_error __mial_list_shrink_to_fit__(void **plist);
enum mial_error __mial_list_grow__(void **plist, uint32_t amount, bool zero_out);
enum mial_error mial_list_shrink(void *list, uint32_t amount);
enum mial_error __mial_list_push__(void **plist, const void *value);
enum mial_error __mial_list_insert_slots__(void **plist, uint32_t index, uint32_t amount, bool zero_out);
enum mial_error __mial_list_insert__(void **plist, uint32_t index, void *value);
enum mial_error mial_list_remove_slots(void *list, uint32_t index, uint32_t amount);
enum mial_error mial_list_clear(void *list);
struct mial_u32 mial_list_size(const void *list);
struct mial_u32 mial_list_capacity(const void *list);
enum mial_error __mial_list_destroy__(void **plist);
#define mial_list_make(T, grow_by) __mial_list_make__(sizeof (T), grow_by)
#define mial_list_reserve(list, amount) __mial_list_reserve__((void **)&(list), amount)
#define mial_list_resize(list, amount) __mial_list_resize__((void **)&(list), amount)
#define mial_list_shrink_to_fit(list) __mial_list_shrink_to_fit__((void **)&(list))
#define mial_list_grow(list, amount, zero_out) __mial_list_grow__((void **)&(list), amount, zero_out)
#define mial_list_insert_slots(list, index, amount, zero_out) __mial_list_insert_slots__((void **)&(list), index, amount, zero_out)
#define mial_list_push_lit(list, ...) __mial_list_push__((void **)&(list), &(typeof (list[0])){ __VA_ARGS__ })
#define mial_list_push_lit_t(list, T, ...) __mial_list_push__((void **)&(list), &(T){ __VA_ARGS__ })
#define mial_list_push_var(list, var) __mial_list_push__((void **)&(list), &var)
#define mial_list_pop(list) mial_list_shrink(list, 1)
#define mial_list_insert_lit(list, index, ...) __mial_list_insert__((void **)&(list), index, &(typeof (list[0])){ __VA_ARGS__ })
#define mial_list_insert_lit_t(list, T, index, ...) __mial_list_insert__((void **)&(list), index, &(T){ __VA_ARGS__ })
#define mial_list_insert_var(list, index, var) __mial_list_insert__((void **)&(list), &var)
#define mial_list_remove(list, index) mial_list_remove_slots(list, index, 1)
#define mial_list_destroy(list) __mial_list_destroy__((void **)&list)
#define mial_list_foreach(list, var) for (typeof (list) var = (list); var < (list) + mial_list_size(list).val; var++)
#define mial_list_foreach_reverse(list, var) for (typeof (list) var = (list) + (int32_t)mial_list_size(list).val - 1; var > (list); var--)
#define mial_list_foreach_t(list, T, var) for (T *var = (T *)(list); var < (T *)((list) + mial_list_size(list).val); var++)
#define mial_list_foreach_reverse_t(list, T, var) for (T *var = (T *)((list) + (int32_t)mial_list_size(list).val - 1); var > (T *)(list); var--)

/* map definitions */
uint32_t mial_hash_string(const char *str, uint32_t size);
struct mial_ptr __mial_map_make__(uint32_t element_size, uint32_t initial_capacity);
struct mial_ptr mial_map_copy(const void *src);
struct mial_u32 __mial_map_insert__(void **pmap, const char *key, uint32_t key_size, bool zero_out);
struct mial_u32 __mial_map_get_index__(const void *map, const char *key, uint32_t key_size);
enum mial_error __mial_map_remove__(void *map, const char *key, uint32_t key_size);
enum mial_error mial_map_clear(void *map);
enum mial_error __mial_map_set__(void **pmap, const char *key, uint32_t key_size, void *value);
struct mial_ptr __mial_map_get_ref__(const void *map, const char *key, uint32_t key_size, void *zero_value);
struct mial_u32 mial_map_size(const void *map);
struct mial_u32 mial_map_capacity(const void *map);
struct mial_bool mial_map_occupied(const void *map, uint32_t index);
struct mial_ptr mial_map_get_key(const void *map, uint32_t index);
struct mial_u32 mial_map_key_default_max_size(const void *map);
enum mial_error mial_map_set_key_default_max_size(const void *map, uint32_t max_size);
struct mial_u32 mial_map_get_key_default_max_size(const void *map);
enum mial_error __mial_map_destroy__(void **pmap);
#define mial_map_make(T, initial_capacity) __mial_map_make__(sizeof (T), initial_capacity)
#define mial_map_insert_n(map, key, key_size, zero_out) __mial_map_insert__((void **)&map, key, key_size, zero_out)
#define mial_map_insert(map, key, zero_out) __mial_map_insert__((void **)&map, key, mial_strlen(key, mial_map_get_key_default_max_size(map).val), zero_out)
#define mial_map_get_index_n(map, key, key_size) __mial_map_get_index__(map, key, key_size)
#define mial_map_get_index(map, key) __mial_map_get_index__(map, key, mial_strlen(key, mial_map_get_key_default_max_size(map).val))
#define mial_map_is_set(map, key) (__mial_map_get_index__(map, key, mial_strlen(key, mial_map_get_key_default_max_size(map).val)).err == MIAL_ERROR_OK)
#define mial_map_is_set_n(map, key, key_size) (__mial_map_get_index__(map, key, key_size).err == MIAL_ERROR_OK)
#define mial_map_remove_n(map, key, key_size) __mial_map_remove__(map, key, key_size)
#define mial_map_remove(map, key) __mial_map_remove__(map, key, mial_strlen(key, mial_map_get_key_default_max_size(map).val))
#define mial_map_set_lit_n(map, key, key_size, ...) __mial_map_set__((void **)&map, key, key_size, &(typeof (map[0])){ __VA_ARGS__ })
#define mial_map_set_lit_nt(map, key, key_size, T, ...) __mial_map_set__((void **)&map, key, key_size, &(T){ __VA_ARGS__ })
#define mial_map_set_lit(map, key, ...) __mial_map_set__((void **)&map, key, mial_strlen(key, mial_map_get_key_default_max_size(map).val), &(typeof (map[0])){ __VA_ARGS__ })
#define mial_map_set_lit_t(map, key, T, ...) __mial_map_set__((void **)&map, key, mial_strlen(key, mial_map_get_key_default_max_size(map).val), &(T){ __VA_ARGS__ })
#define mial_map_set_var_n(map, key, key_size, var) __mial_map_set__((void **)&map, key, key_size, &var)
#define mial_map_set_var(map, key, var) __mial_map_set__((void **)&map, key, mial_strlen(key, mial_map_get_key_default_max_size(map).val), &var)
#define mial_map_get_ref_n(map, key, key_size) __mial_map_get_ref__(map, key, key_size, 0)
#define mial_map_get_ref(map, key) __mial_map_get_ref__(map, key, mial_strlen(key, mial_map_get_key_default_max_size(map).val), 0)
#define mial_map_get_n(map, key, key_size) (*(typeof(map))__mial_map_get_ref__(map, key, key_size, &(typeof (map[0])) { 0 }).val)
#define mial_map_get_nt(map, key, key_size, T) (*(T *)__mial_map_get_ref__(map, key, key_size, &(T) { 0 }).val)
#define mial_map_get(map, key) (*(typeof(map))__mial_map_get_ref__(map, key, mial_strlen(key, mial_map_get_key_default_max_size(map).val), &(typeof (map[0])) { 0 }).val)
#define mial_map_get_t(map, key, T) (*(T *)__mial_map_get_ref__(map, key, mial_strlen(key, mial_map_get_key_default_max_size(map).val), &(T) { 0 }).val)
#define mial_map_destroy(map) __mial_map_destroy__((void **)&map)
#define mial_map_foreach(map, var) for (uint32_t __mial_i__ = 0; __mial_i__ < mial_list_capacity(map).val; __mial_i__++) \
                                     for (struct { const char *key; typeof (map) val; } var = {\
                                           mial_map_get_key(map, __mial_i__).val, &(map)[__mial_i__]\
                                         };\
                                         var == &(map)[__mial_i__] && mial_map_occupied(map, __mial_i__);\
                                         var++)

/* implementations */
#ifdef __MIAL_IMPL__
#include <string.h>

#ifndef mial_malloc
#  define mial_malloc malloc
#endif
#ifndef mial_realloc
#  define mial_realloc realloc
#endif
#ifndef mial_free
#  define mial_free free
#endif

/* helper functions implementation */
uint32_t
mial_strlen(const char *s, uint32_t max) {
  for (uint32_t i = 0; i < max; i++) if (s[i] == '\0') return i;
  return max;
}

/* list implementation */
struct __mial_list_header__ {
  uint32_t size;
  uint32_t capa;
  uint32_t grow;
  uint32_t elem;
  uint8_t  data[];
};
#define MIAL_LIST_GET_HEADER(LIST) (((struct __mial_list_header__ *)(LIST)) - 1)

struct mial_ptr
__mial_list_make__(uint32_t element_size, uint32_t grow_by) {
  if (element_size == 0) return (struct mial_ptr) { 0, MIAL_ERROR_AMOUNT };
  struct __mial_list_header__ *h = (struct __mial_list_header__ *)mial_malloc(sizeof (struct __mial_list_header__) + element_size);
  if (!h) return (struct mial_ptr) { 0, MIAL_ERROR_ALLOC };
  h->size = 0;
  h->capa = 1;
  h->grow = grow_by;
  h->elem = element_size;
  return (struct mial_ptr) { h->data, MIAL_ERROR_OK };
}

struct mial_ptr
mial_list_copy(const void *src) {
  if (!src) return (struct mial_ptr) { 0, MIAL_ERROR_NULL };
  struct __mial_list_header__ *src_h = MIAL_LIST_GET_HEADER(src);
  uint32_t alloc_size = sizeof (struct __mial_list_header__) + src_h->elem * src_h->capa;
  struct __mial_list_header__ *dest_h = (struct __mial_list_header__ *)mial_malloc(alloc_size);
  if (!dest_h) return (struct mial_ptr) { 0, MIAL_ERROR_ALLOC };
  memcpy(dest_h, src_h, alloc_size);
  return (struct mial_ptr) { dest_h->data, MIAL_ERROR_OK };
}

enum mial_error
__mial_list_reserve__(void **plist, uint32_t amount) {
  if (!plist || !*plist) return MIAL_ERROR_NULL;
  struct __mial_list_header__ *h = MIAL_LIST_GET_HEADER(*plist);
  if (h->capa >= amount) return MIAL_ERROR_OK;
  struct __mial_list_header__ *new_h = (struct __mial_list_header__ *)mial_realloc(h, sizeof (struct __mial_list_header__) + h->elem * amount);
  if (!new_h) return MIAL_ERROR_ALLOC;
  h = new_h;
  *plist = h->data;
  h->capa = amount;
  return MIAL_ERROR_OK;
}

enum mial_error
__mial_list_resize__(void **plist, uint32_t amount) {
  enum mial_error err = __mial_list_reserve__(plist, amount);
  if (err != MIAL_ERROR_OK) return err;
  struct __mial_list_header__ *h = MIAL_LIST_GET_HEADER(*plist);
  h->size = amount;
  return err;
}

enum mial_error
__mial_list_shrink_to_fit__(void **plist) {
  if (!plist || !*plist) return MIAL_ERROR_NULL;
  struct __mial_list_header__ *h = MIAL_LIST_GET_HEADER(*plist);
  uint32_t new_capa = h->size == 0 ? 1 : h->size;
  struct __mial_list_header__ *new_h = (struct __mial_list_header__ *)mial_realloc(h, sizeof (struct __mial_list_header__) + h->elem * new_capa);
  if (!new_h) return MIAL_ERROR_ALLOC;
  h = new_h;
  *plist = h->data;
  h->capa = new_capa;
  return MIAL_ERROR_OK;
}

enum mial_error
__mial_list_grow__(void **plist, uint32_t amount, bool zero_out) {
  if (!plist || !*plist) return MIAL_ERROR_NULL;
  struct __mial_list_header__ *h = MIAL_LIST_GET_HEADER(*plist);
  if (h->size + amount > h->capa) {
    uint32_t new_capa = h->capa;
    while (new_capa < h->size + amount) new_capa = h->grow ? new_capa + h->grow : new_capa * 2;
    struct __mial_list_header__ *new_h = (struct __mial_list_header__ *)mial_realloc(h, sizeof(struct __mial_list_header__) + h->elem * new_capa);
    if (!new_h) return MIAL_ERROR_ALLOC;
    h = new_h;
    h->capa = new_capa;
    *plist = h->data;
  }
  if (zero_out) memset(&h->data[h->elem * h->size], 0, h->elem * amount);
  h->size += amount;
  return MIAL_ERROR_OK;
}

enum mial_error
mial_list_shrink(void *list, uint32_t amount) {
  if (!list) return MIAL_ERROR_NULL;
  struct __mial_list_header__ *h = MIAL_LIST_GET_HEADER(list);
  if (h->size < amount || amount == 0) return h->size == 0 ? MIAL_ERROR_EMPTY : MIAL_ERROR_AMOUNT;
  h->size -= amount;
  return MIAL_ERROR_OK;
}

enum mial_error
__mial_list_push__(void **plist, const void *value) {
  if (!value) return MIAL_ERROR_NULL;
  enum mial_error err = __mial_list_grow__(plist, 1, false);
  if (err != MIAL_ERROR_OK) return err;
  struct __mial_list_header__ *h = MIAL_LIST_GET_HEADER(*plist);
  memcpy(&h->data[h->elem * (h->size - 1)], value, h->elem);
  return MIAL_ERROR_OK;
}

enum mial_error
__mial_list_insert_slots__(void **plist, uint32_t index, uint32_t amount, bool zero_out) {
  if (!plist || !*plist) return MIAL_ERROR_NULL;
  if (amount == 0) return MIAL_ERROR_AMOUNT;
  struct __mial_list_header__ *h = MIAL_LIST_GET_HEADER(*plist);
  if (index > h->size) return MIAL_ERROR_INDEX;
  if (index == h->size) return __mial_list_grow__(plist, amount, zero_out);
  uint32_t old_size = h->size;
  enum mial_error err = __mial_list_grow__(plist, amount, false);
  if (err != MIAL_ERROR_OK) return err;
  h = MIAL_LIST_GET_HEADER(*plist);
  memmove(&h->data[h->elem * (index + amount)], &h->data[h->elem * index], h->elem * (old_size - index));
  if (zero_out) memset(&h->data[h->elem * index], 0, h->elem * amount);
  return MIAL_ERROR_OK;
}

enum mial_error
__mial_list_insert__(void **plist, uint32_t index, void *value) {
  if (!value) return MIAL_ERROR_NULL;
  enum mial_error err = __mial_list_insert_slots__(plist, index, 1, false);
  if (err != MIAL_ERROR_OK) return err;
  struct __mial_list_header__ *h = MIAL_LIST_GET_HEADER(*plist);
  memcpy(&h->data[h->elem * index], value, h->elem);
  return MIAL_ERROR_OK;
}

enum mial_error
mial_list_remove_slots(void *list, uint32_t index, uint32_t amount) {
  if (!list) return MIAL_ERROR_NULL;
  struct __mial_list_header__ *h = MIAL_LIST_GET_HEADER(list);
  if (h->size == 0) return MIAL_ERROR_EMPTY;
  if (index >= h->size) return MIAL_ERROR_INDEX;
  if (amount == 0 || index + amount > h->size) return MIAL_ERROR_AMOUNT;
  h->size -= amount;
  memmove(&h->data[h->elem * index], &h->data[h->elem * (index + amount)], h->elem * (h->size - index));
  return MIAL_ERROR_OK;
}

enum mial_error
mial_list_clear(void *list) {
  if (!list) return MIAL_ERROR_NULL;
  struct __mial_list_header__ *h = MIAL_LIST_GET_HEADER(list);
  h->size = 0;
  return MIAL_ERROR_OK;
}

struct mial_u32
mial_list_size(const void *list) {
  if (!list) return (struct mial_u32) { 0, MIAL_ERROR_NULL };
  return (struct mial_u32) { MIAL_LIST_GET_HEADER(list)->size, MIAL_ERROR_OK };
}

struct mial_u32
mial_list_capacity(const void *list) {
  if (!list) return (struct mial_u32) { 0, MIAL_ERROR_NULL };
  return (struct mial_u32) { MIAL_LIST_GET_HEADER(list)->capa, MIAL_ERROR_OK };
}

enum mial_error
__mial_list_destroy__(void **plist) {
  if (!plist || !*plist) return MIAL_ERROR_NULL;
  free(MIAL_LIST_GET_HEADER(*plist));
  *plist = 0;
  return MIAL_ERROR_OK;
}

#undef MIAL_LIST_GET_HEADER
#define __mial_list_header__

/* map implementation */
struct __mial_map_key_storage__ {
  uint32_t size;
  char data[];
};
struct __mial_map_header__ {
  uint32_t size;
  uint32_t capa;
  uint32_t elem;
  uint32_t key_max;
  bool *occupied;
  uint32_t *key_index;
  char *key_buffer;
  uint8_t data[];
};
#define MIAL_MAP_GET_HEADER(MAP) (((struct __mial_map_header__ *)(MAP)) - 1)

uint32_t
mial_hash_string(const char *str, uint32_t size) { /* djb2 */
  uint32_t hash = 5381;
  for (uint32_t i = 0; i < size; i++) hash = ((hash << 5) + hash) + str[i]; /* hash * 33 + c */
  return hash;
}

struct mial_ptr
__mial_map_make__(uint32_t element_size, uint32_t initial_capacity) {
  if (element_size == 0) return (struct mial_ptr) { 0, MIAL_ERROR_AMOUNT };
  if (!initial_capacity) initial_capacity = 13;
  struct __mial_map_header__ *h = (struct __mial_map_header__ *)
    mial_malloc(sizeof (struct __mial_map_header__) + initial_capacity * (element_size + sizeof (uint32_t) + sizeof (bool)));
  h->size = 0;
  h->capa = initial_capacity;
  h->elem = element_size;
  h->occupied = (bool *)&h->data[h->elem * h->capa];
  h->key_index = (uint32_t *)&h->occupied[h->capa];
  h->key_max = 1024;
  struct mial_ptr list_res = mial_list_make(uint8_t, 0);
  if (list_res.err != MIAL_ERROR_OK) {
    free(h);
    return list_res;
  }
  h->key_buffer = list_res.val;
  memset(h->occupied, false, sizeof (bool) * h->capa);
  return (struct mial_ptr) { h->data, MIAL_ERROR_OK };
}

struct mial_ptr
mial_map_copy(const void *src) {
  if (!src) return (struct mial_ptr) { 0, MIAL_ERROR_NULL };
  struct __mial_map_header__ *h = MIAL_MAP_GET_HEADER(src);
  uint32_t alloc_size = sizeof (struct __mial_map_header__) + h->capa * (h->elem + sizeof (uint32_t) + sizeof (bool));
  struct __mial_map_header__ *dest_h = (struct __mial_map_header__ *)mial_malloc(alloc_size);
  if (!dest_h) return (struct mial_ptr) { 0, MIAL_ERROR_ALLOC };
  memcpy(dest_h, h, alloc_size);
  dest_h->occupied = (bool *)&dest_h->data[dest_h->elem * dest_h->capa];
  dest_h->key_index = (uint32_t *)&dest_h->occupied[dest_h->capa];
  struct mial_ptr key_buffer = mial_list_copy(h->key_buffer);
  if (key_buffer.err != MIAL_ERROR_OK) {
    free(dest_h);
    return (struct mial_ptr) { 0, MIAL_ERROR_ALLOC };
  }
  dest_h->key_buffer = key_buffer.val;
  return (struct mial_ptr) { dest_h->data, MIAL_ERROR_OK };
}

struct mial_u32
__mial_map_insert__(void **pmap, const char *key, uint32_t key_size, bool zero_out) {
  if (!pmap || !*pmap || !key) return (struct mial_u32) { 0, MIAL_ERROR_NULL };
  if (!key_size) return (struct mial_u32) { 0, MIAL_ERROR_AMOUNT };
  struct __mial_map_header__ *h = MIAL_MAP_GET_HEADER(*pmap);
  if ((float)h->size / (float)h->capa > 0.75f || h->size == h->capa) {
    /* rehash */
    uint32_t new_capa = h->capa * 2;
    struct __mial_map_header__ *new_h = (struct __mial_map_header__ *)
      mial_malloc(sizeof (struct __mial_map_header__) + new_capa * (h->elem + sizeof (uint32_t) + sizeof (bool)));
    if (!new_h) return (struct mial_u32) { 0, MIAL_ERROR_ALLOC };
    new_h->size = h->size;
    new_h->capa = new_capa;
    new_h->elem = h->elem;
    new_h->key_max = h->key_max;
    new_h->occupied = (bool *)&new_h->data[new_h->elem * new_h->capa];
    new_h->key_index = (uint32_t *)&new_h->occupied[new_h->capa];
    new_h->key_buffer = h->key_buffer;
    memset(new_h->occupied, false, sizeof (bool) * new_h->capa);
    for (uint32_t i = 0; i < h->capa; i++) {
      if (!h->occupied[i]) continue;
      struct __mial_map_key_storage__ *cur_key = (struct __mial_map_key_storage__ *)&h->key_buffer[h->key_index[i]];
      uint32_t id = mial_hash_string(cur_key->data, cur_key->size) % new_h->capa;
      uint32_t loop_id = id;
      while (new_h->occupied[id]) {
        struct __mial_map_key_storage__ *occupied_key = (struct __mial_map_key_storage__ *)&new_h->key_buffer[new_h->key_index[id]];
        if (cur_key->size == occupied_key->size && strncmp(cur_key->data, occupied_key->data, cur_key->size) == 0) {
          free(new_h);
          return (struct mial_u32) { 0, MIAL_ERROR_CORRUPTED };
        }
        id = (id + 1) % new_h->capa;
        if (loop_id == id) {
          free(new_h);
          return (struct mial_u32) { 0, MIAL_ERROR_FULL };
        }
      }
      new_h->key_index[id] = h->key_index[i];
      new_h->occupied[id] = true;
      memcpy(&new_h->data[new_h->elem * id], &h->data[new_h->elem * i], new_h->elem);
    }
    free(h);
    h = new_h;
    *pmap = h->data;
  }
  uint32_t id = mial_hash_string(key, key_size) % h->capa;
  uint32_t loop_id = id;
  while (h->occupied[id]) {
    struct __mial_map_key_storage__ *occupied_key = (struct __mial_map_key_storage__ *)&h->key_buffer[h->key_index[id]];
    if (key_size == occupied_key->size && strncmp(key, occupied_key->data, key_size) == 0) return (struct mial_u32) { 0, MIAL_ERROR_EXISTS };
    id = (id + 1) % h->capa;
    if (loop_id == id) return (struct mial_u32) { 0, MIAL_ERROR_FULL };
  }
  h->key_index[id] = mial_list_size(h->key_buffer).val;
  enum mial_error err = mial_list_grow(h->key_buffer, sizeof (uint32_t) + key_size + 1, false);
  if (err != MIAL_ERROR_OK) return (struct mial_u32) { 0, err };
  struct __mial_map_key_storage__ *key_storage = (struct __mial_map_key_storage__ *)&h->key_buffer[h->key_index[id]];
  key_storage->size = key_size;
  memcpy(key_storage->data, key, key_size);
  key_storage->data[key_size] = '\0';
  h->occupied[id] = true;
  if (zero_out) memset(&h->data[h->elem * id], 0, h->elem);
  h->size++;
  return (struct mial_u32) { id, MIAL_ERROR_OK };
}

struct mial_u32
__mial_map_get_index__(const void *map, const char *key, uint32_t key_size) {
  if (!map || !key) return (struct mial_u32) { 0, MIAL_ERROR_NULL };
  if (!key_size) return (struct mial_u32) { 0, MIAL_ERROR_AMOUNT };
  struct __mial_map_header__ *h = MIAL_MAP_GET_HEADER(map);
  uint32_t id = mial_hash_string(key, key_size) % h->capa;
  uint32_t loop_id = id;
  while (h->occupied[id]) {
    struct __mial_map_key_storage__ *occupied_key = (struct __mial_map_key_storage__ *)&h->key_buffer[h->key_index[id]];
    if (key_size == occupied_key->size && strncmp(key, occupied_key->data, key_size) == 0) return (struct mial_u32) { id, MIAL_ERROR_OK };
    id = (id + 1) % h->capa;
    if (loop_id == id) return (struct mial_u32) { 0, MIAL_ERROR_DONT_EXISTS };
  }
  return (struct mial_u32) { 0, MIAL_ERROR_DONT_EXISTS };
}

enum mial_error
__mial_map_remove__(void *map, const char *key, uint32_t key_size) {
  if (!map || !key) return MIAL_ERROR_NULL;
  if (!key_size) return MIAL_ERROR_AMOUNT;
  struct mial_u32 id = __mial_map_get_index__(map, key, key_size);
  if (id.err != MIAL_ERROR_OK) return id.err;
  struct __mial_map_header__ *h = MIAL_MAP_GET_HEADER(map);
  uint32_t amount_to_remove = sizeof (uint32_t) + key_size + 1;
  mial_list_remove_slots(h->key_buffer, h->key_index[id.val], amount_to_remove);
  h->occupied[id.val] = false;
  for (uint32_t i = 0; i < h->capa; i++) {
    if (!h->occupied[i]) continue;
    if (h->key_index[i] > h->key_index[id.val]) h->key_index[i] -= amount_to_remove;
  }
  h->size--;
  return MIAL_ERROR_OK;
}

enum mial_error
mial_map_clear(void *map) {
  if (!map) return MIAL_ERROR_NULL;
  struct __mial_map_header__ *h = MIAL_MAP_GET_HEADER(map);
  mial_list_clear(h->key_buffer);
  memset(h->occupied, false, h->capa);
  h->size = 0;
  return MIAL_ERROR_OK;
}

enum mial_error
__mial_map_set__(void **pmap, const char *key, uint32_t key_size, void *value) {
  if (!pmap || !*pmap || !key) return MIAL_ERROR_NULL;
  if (!key_size) return MIAL_ERROR_AMOUNT;
  struct mial_u32 id = __mial_map_get_index__(*pmap, key, key_size);
  if (id.err != MIAL_ERROR_OK) {
    if (id.err != MIAL_ERROR_DONT_EXISTS) return id.err;
    id = __mial_map_insert__(pmap, key, key_size, false);
    if (id.err != MIAL_ERROR_OK) return id.err;
  }
  struct __mial_map_header__ *h = MIAL_MAP_GET_HEADER(*pmap);
  memcpy(&h->data[h->elem * id.val], value, h->elem);
  return MIAL_ERROR_OK;
}

struct mial_ptr
__mial_map_get_ref__(const void *map, const char *key, uint32_t key_size, void *zero_value) {
  if (!map || !key) return (struct mial_ptr) { zero_value, MIAL_ERROR_NULL };
  if (!key_size) return (struct mial_ptr) { zero_value, MIAL_ERROR_AMOUNT };
  struct mial_u32 id = __mial_map_get_index__(map, key, key_size);
  if (id.err != MIAL_ERROR_OK) return (struct mial_ptr) { zero_value, MIAL_ERROR_DONT_EXISTS };
  struct __mial_map_header__ *h = MIAL_MAP_GET_HEADER(map);
  return (struct mial_ptr) { &h->data[h->elem * id.val], MIAL_ERROR_OK };
}

struct mial_u32
mial_map_size(const void *map) {
  if (!map) return (struct mial_u32) { 0, MIAL_ERROR_NULL };
  return (struct mial_u32) { MIAL_MAP_GET_HEADER(map)->size, MIAL_ERROR_OK };
}

struct mial_u32
mial_map_capacity(const void *map) {
  if (!map) return (struct mial_u32) { 0, MIAL_ERROR_NULL };
  return (struct mial_u32) { MIAL_MAP_GET_HEADER(map)->capa, MIAL_ERROR_OK };
}

struct mial_bool
mial_map_occupied(const void *map, uint32_t index) {
  if (!map) return (struct mial_bool) { false, MIAL_ERROR_NULL };
  struct __mial_map_header__ *h = MIAL_MAP_GET_HEADER(map);
  if (index >= h->capa) return (struct mial_bool) { false, MIAL_ERROR_INDEX };
  return (struct mial_bool) { h->occupied[index], MIAL_ERROR_OK };
}

struct mial_ptr
mial_map_get_key(const void *map, uint32_t index) {
  if (!map) return (struct mial_ptr) { 0, MIAL_ERROR_NULL };
  struct __mial_map_header__ *h = MIAL_MAP_GET_HEADER(map);
  if (index >= h->capa) return (struct mial_ptr) { 0, MIAL_ERROR_INDEX };
  return (struct mial_ptr) { ((struct __mial_map_key_storage__ *)(&h->key_buffer[h->key_index[index]]))->data, MIAL_ERROR_OK };
}

enum mial_error
mial_map_set_key_default_max_size(const void *map, uint32_t max_size) {
  if (!map) return MIAL_ERROR_NULL;
  struct __mial_map_header__ *h = MIAL_MAP_GET_HEADER(map);
  h->key_max = max_size;
  return MIAL_ERROR_OK;
}

struct mial_u32
mial_map_get_key_default_max_size(const void *map) {
  if (!map) return (struct mial_u32) { 0, MIAL_ERROR_NULL };
  struct __mial_map_header__ *h = MIAL_MAP_GET_HEADER(map);
  return (struct mial_u32) { h->key_max, MIAL_ERROR_OK };
}

enum mial_error
__mial_map_destroy__(void **pmap) {
  if (!pmap || !*pmap) return MIAL_ERROR_NULL;
  struct __mial_map_header__ *h = MIAL_MAP_GET_HEADER(*pmap);
  mial_list_destroy(h->key_buffer);
  free(h);
  *pmap = 0;
  return MIAL_ERROR_OK;
}

#undef MIAL_MAP_GET_HEADER
#define __mial_map_key_storage__
#define __mial_map_header__
#endif

#endif/*__MIAL_H__*/
