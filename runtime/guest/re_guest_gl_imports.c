#include "re_guest_gl_imports.h"

enum {
    RE_SYMBOL_GL_TEX_PARAMETERI = 0,
    RE_SYMBOL_GL_TEX_PARAMETERX = 1
};

static REGUESTIMPORTStatus gl_tex_parameter_i_handler(
    const S3EAAPCS32Frame *f, const REGUESTMEM *m, void *u,
    REGUESTIMPORTResult *r) {
    REGUESTGLBackend *b=(REGUESTGLBackend *)u;
    (void)m;
    if(!f||!b||!b->tex_parameter_i||!r) return RE_IMPORT_E_HANDLER;
    b->tex_parameter_i(f->r[0],f->r[1],(int32_t)f->r[2]);
    r->r0=0; r->r1=0;
    return RE_IMPORT_OK;
}
static REGUESTIMPORTStatus gl_tex_parameter_x_handler(
    const S3EAAPCS32Frame *f, const REGUESTMEM *m, void *u,
    REGUESTIMPORTResult *r) {
    REGUESTGLBackend *b=(REGUESTGLBackend *)u;
    (void)m;
    if(!f||!b||!b->tex_parameter_x||!r) return RE_IMPORT_E_HANDLER;
    b->tex_parameter_x(f->r[0],f->r[1],(int32_t)f->r[2]);
    r->r0=0; r->r1=0;
    return RE_IMPORT_OK;
}

static REGUESTIMPORTStatus gl_tex_parameter_i_mapped_handler(
    const S3EAAPCS32Frame *f,const REGUESTMEMBUS *bus,void *u,
    REGUESTIMPORTResult *r) {
    REGUESTGLBackend *b=(REGUESTGLBackend *)u;
    (void)bus;
    if(!f||!b||!b->tex_parameter_i||!r)return RE_IMPORT_E_HANDLER;
    b->tex_parameter_i(f->r[0],f->r[1],(int32_t)f->r[2]);
    r->r0=0;r->r1=0;
    return RE_IMPORT_OK;
}
static REGUESTIMPORTStatus gl_tex_parameter_x_mapped_handler(
    const S3EAAPCS32Frame *f,const REGUESTMEMBUS *bus,void *u,
    REGUESTIMPORTResult *r) {
    REGUESTGLBackend *b=(REGUESTGLBackend *)u;
    (void)bus;
    if(!f||!b||!b->tex_parameter_x||!r)return RE_IMPORT_E_HANDLER;
    b->tex_parameter_x(f->r[0],f->r[1],(int32_t)f->r[2]);
    r->r0=0;r->r1=0;
    return RE_IMPORT_OK;
}
REGUESTIMPORTStatus re_guest_gl_register_verified_mapped(
    REGUESTIMPORTTable *t,REGUESTGLBackend *b,uint32_t *pi,uint32_t *px) {
    REGUESTIMPORTStatus s;
    if(!t||!b||!pi||!px||!b->tex_parameter_i||!b->tex_parameter_x)
        return RE_IMPORT_E_ARGUMENT;
    s=re_guest_import_bind_mapped(t,RE_SYMBOL_GL_TEX_PARAMETERI,
        gl_tex_parameter_i_mapped_handler,b,pi);
    if(s!=RE_IMPORT_OK)return s;
    return re_guest_import_bind_mapped(t,RE_SYMBOL_GL_TEX_PARAMETERX,
        gl_tex_parameter_x_mapped_handler,b,px);
}
static REGUESTIMPORTStatus patch_one(REGUESTIMPORTTable *t, REGUESTMEM *m,
                                     uint32_t id, uint32_t image_base,
                                     uint32_t offset) {
    if(image_base>UINT32_MAX-offset) return RE_IMPORT_E_MEMORY;
    return re_guest_import_patch_slot(t,id,m,image_base+offset);
}

REGUESTIMPORTStatus re_guest_gl_bind_verified(REGUESTIMPORTTable *t,
                                               REGUESTMEM *m,
                                               REGUESTGLBackend *b,
                                               uint32_t image_base,
                                               uint32_t *pi,
                                               uint32_t *px) {
    REGUESTIMPORTStatus s;
    if(!t||!m||!b||!pi||!px||!b->tex_parameter_i||!b->tex_parameter_x)
        return RE_IMPORT_E_ARGUMENT;
    s=re_guest_import_bind(t,RE_SYMBOL_GL_TEX_PARAMETERI,
                           gl_tex_parameter_i_handler,b,pi);
    if(s!=RE_IMPORT_OK) return s;
    s=re_guest_import_bind(t,RE_SYMBOL_GL_TEX_PARAMETERX,
                           gl_tex_parameter_x_handler,b,px);
    if(s!=RE_IMPORT_OK) return s;
    /* Stage 16 relocation registry: two slots per verified symbol. */
    const uint32_t i_slots[]={0x000C7C4Cu,0x0013B960u};
    const uint32_t x_slots[]={0x000C7C50u,0x0013B8C0u};
    for(size_t i=0;i<2;i++) {
        s=patch_one(t,m,RE_SYMBOL_GL_TEX_PARAMETERI,image_base,i_slots[i]);
        if(s!=RE_IMPORT_OK) return s;
        s=patch_one(t,m,RE_SYMBOL_GL_TEX_PARAMETERX,image_base,x_slots[i]);
        if(s!=RE_IMPORT_OK) return s;
    }
    return RE_IMPORT_OK;
}
