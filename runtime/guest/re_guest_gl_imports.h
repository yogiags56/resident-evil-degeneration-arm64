#ifndef RE_GUEST_GL_IMPORTS_H
#define RE_GUEST_GL_IMPORTS_H
#include <stdint.h>
#include "re_guest_imports.h"

typedef void (*REGUESTGLTexParameteriFn)(uint32_t target, uint32_t pname, int32_t param);
typedef void (*REGUESTGLTexParameterxFn)(uint32_t target, uint32_t pname, int32_t param);

typedef struct {
    REGUESTGLTexParameteriFn tex_parameter_i;
    REGUESTGLTexParameterxFn tex_parameter_x;
    void *user;
} REGUESTGLBackend;

/* Register and patch only the two graphics imports whose prototypes and
 * call-site use were verified in prior stages. Guest image base must match
 * the loaded XE3U image base; slot offsets are image-relative.
 */
REGUESTIMPORTStatus re_guest_gl_bind_verified(REGUESTIMPORTTable *table,
                                               REGUESTMEM *memory,
                                               REGUESTGLBackend *backend,
                                               uint32_t image_base,
                                               uint32_t *tex_parameter_i_ptr,
                                               uint32_t *tex_parameter_x_ptr);

#endif
