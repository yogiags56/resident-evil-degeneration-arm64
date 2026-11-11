#include "re_guest_elf32_dynamic.h"
#include <string.h>
static uint32_t read_u32(const uint8_t *p,int little) {
 if(little)return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
 return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|(uint32_t)p[3];
}
static int map_file_backed(const REGUESTELF32IMAGE *image,uint32_t address,
 uint32_t length,uint32_t *file_offset) {
 size_t matches=0u; uint32_t result=0u;
 uint64_t requested_end=(uint64_t)address+length;
 if(requested_end>0x100000000ULL)return 0;
 for(size_t i=0;i<image->program_header_count;i++){
  const REGUESTELF32PROGRAMHEADER *p=&image->program_headers[i];
  uint64_t start=p->virtual_address,end=start+p->file_size;
  if(p->type!=RE_GUEST_ELF32_PT_LOAD)continue;
  if((uint64_t)address>=start&&requested_end<=end){
   uint64_t offset=(uint64_t)p->offset+(address-p->virtual_address);
   if(offset>0xffffffffULL)return 0;
   result=(uint32_t)offset;matches++;
  }
 }
 if(matches!=1u)return 0;
 *file_offset=result;return 1;
}
static REGUESTELF32DYNAMICStatus copy_string(const uint8_t *bytes,size_t size,
 uint32_t table_offset,uint32_t table_size,uint32_t string_offset,
 char out[RE_GUEST_ELF32_DYNAMIC_STRING_CAP]) {
 uint64_t start=(uint64_t)table_offset+string_offset;
 uint32_t remaining;
 const uint8_t *end;
 size_t length;
 if(string_offset>=table_size||start>size)return RE_GUEST_ELF32_DYNAMIC_E_STRING;
 remaining=table_size-string_offset;
 if(start+remaining>size)return RE_GUEST_ELF32_DYNAMIC_E_RANGE;
 end=(const uint8_t *)memchr(bytes+(size_t)start,0,remaining);
 if(!end)return RE_GUEST_ELF32_DYNAMIC_E_STRING;
 length=(size_t)(end-(bytes+(size_t)start));
 if(!length||length>=RE_GUEST_ELF32_DYNAMIC_STRING_CAP)
  return RE_GUEST_ELF32_DYNAMIC_E_STRING;
 memcpy(out,bytes+(size_t)start,length);out[length]='\0';
 return RE_GUEST_ELF32_DYNAMIC_OK;
}
REGUESTELF32DYNAMICStatus re_guest_elf32_parse_dynamic(
 const uint8_t *bytes,size_t size,const REGUESTELF32IMAGE *image,
 REGUESTELF32DYNAMIC *dynamic,size_t *failed_entry) {
 const REGUESTELF32PROGRAMHEADER *segment=NULL;
 uint32_t needed_offsets[RE_GUEST_ELF32_MAX_NEEDED];
 uint32_t dynamic_file_offset=0u;
 uint32_t soname_offset=0u,strtab=0u,strsz=0u,strtab_file=0u;
 uint8_t has_init=0u,has_fini=0u,has_ia=0u,has_iasz=0u,has_fa=0u,has_fasz=0u;
 uint32_t init=0u,fini=0u,ia=0u,iasz=0u,fa=0u,fasz=0u,file_check=0u;
 uint8_t has_strtab=0u,has_strsz=0u,has_soname=0u,terminated=0u;
 size_t dynamic_count=0u;
 if(failed_entry)*failed_entry=0u;
 if(!bytes||!image||!dynamic||image->program_header_count>RE_GUEST_ELF32_MAX_PROGRAM_HEADERS)
  return RE_GUEST_ELF32_DYNAMIC_E_ARGUMENT;
 memset(dynamic,0,sizeof(*dynamic));
 for(size_t i=0;i<image->program_header_count;i++){
  const REGUESTELF32PROGRAMHEADER *p=&image->program_headers[i];
  if(p->type==RE_GUEST_ELF32_PT_DYNAMIC){segment=p;dynamic_count++;}
 }
 if(!dynamic_count)return RE_GUEST_ELF32_DYNAMIC_OK;
 if(dynamic_count!=1u)return RE_GUEST_ELF32_DYNAMIC_E_MULTIPLE;
 if(!segment->file_size||segment->file_size%8u||
    (uint64_t)segment->offset+segment->file_size>size)
  return RE_GUEST_ELF32_DYNAMIC_E_SIZE;
 /* PT_DYNAMIC must be the same file-backed bytes exposed by one PT_LOAD. */
 if(!map_file_backed(image,segment->virtual_address,segment->file_size,
       &dynamic_file_offset)||dynamic_file_offset!=segment->offset)
  return RE_GUEST_ELF32_DYNAMIC_E_RANGE;
 if(segment->file_size/8u>RE_GUEST_ELF32_MAX_DYNAMIC_ENTRIES)
  return RE_GUEST_ELF32_DYNAMIC_E_CAPACITY;
 dynamic->present=1u;
 for(size_t i=0;i<segment->file_size/8u;i++){
  const uint8_t *p=bytes+segment->offset+i*8u;
  uint32_t tag=read_u32(p,image->little_endian);
  uint32_t value=read_u32(p+4u,image->little_endian);
  dynamic->entries[i].tag=tag;dynamic->entries[i].value=value;
  dynamic->entry_count++;
  if(tag==RE_GUEST_ELF32_DT_NULL){terminated=1u;break;}
  if(tag==RE_GUEST_ELF32_DT_NEEDED){
   if(dynamic->needed_count>=RE_GUEST_ELF32_MAX_NEEDED){
    if(failed_entry)*failed_entry=i;return RE_GUEST_ELF32_DYNAMIC_E_CAPACITY;
   }
   needed_offsets[dynamic->needed_count++]=value;
  } else if(tag==RE_GUEST_ELF32_DT_SONAME){
   if(has_soname){if(failed_entry)*failed_entry=i;return RE_GUEST_ELF32_DYNAMIC_E_DUPLICATE_TAG;}
   has_soname=1u;soname_offset=value;
  } else if(tag==RE_GUEST_ELF32_DT_STRTAB){
   if(has_strtab){if(failed_entry)*failed_entry=i;return RE_GUEST_ELF32_DYNAMIC_E_DUPLICATE_TAG;}
   has_strtab=1u;strtab=value;
  } else if(tag==RE_GUEST_ELF32_DT_INIT){
   if(has_init){if(failed_entry)*failed_entry=i;return RE_GUEST_ELF32_DYNAMIC_E_DUPLICATE_TAG;}
   has_init=1u;init=value;
  } else if(tag==RE_GUEST_ELF32_DT_FINI){
   if(has_fini){if(failed_entry)*failed_entry=i;return RE_GUEST_ELF32_DYNAMIC_E_DUPLICATE_TAG;}
   has_fini=1u;fini=value;
  } else if(tag==RE_GUEST_ELF32_DT_INIT_ARRAY){
   if(has_ia){if(failed_entry)*failed_entry=i;return RE_GUEST_ELF32_DYNAMIC_E_DUPLICATE_TAG;}
   has_ia=1u;ia=value;
  } else if(tag==RE_GUEST_ELF32_DT_INIT_ARRAYSZ){
   if(has_iasz){if(failed_entry)*failed_entry=i;return RE_GUEST_ELF32_DYNAMIC_E_DUPLICATE_TAG;}
   has_iasz=1u;iasz=value;
  } else if(tag==RE_GUEST_ELF32_DT_FINI_ARRAY){
   if(has_fa){if(failed_entry)*failed_entry=i;return RE_GUEST_ELF32_DYNAMIC_E_DUPLICATE_TAG;}
   has_fa=1u;fa=value;
  } else if(tag==RE_GUEST_ELF32_DT_FINI_ARRAYSZ){
   if(has_fasz){if(failed_entry)*failed_entry=i;return RE_GUEST_ELF32_DYNAMIC_E_DUPLICATE_TAG;}
   has_fasz=1u;fasz=value;
  } else if(tag==RE_GUEST_ELF32_DT_STRSZ){
   if(has_strsz){if(failed_entry)*failed_entry=i;return RE_GUEST_ELF32_DYNAMIC_E_DUPLICATE_TAG;}
   has_strsz=1u;strsz=value;
  }
 }
 if(!terminated){if(failed_entry)*failed_entry=dynamic->entry_count;return RE_GUEST_ELF32_DYNAMIC_E_TERMINATOR;}
 if(has_ia!=has_iasz||has_fa!=has_fasz||
    (has_iasz&&(iasz%4u)!=0u)||(has_fasz&&(fasz%4u)!=0u))
  return RE_GUEST_ELF32_DYNAMIC_E_SIZE;
 if(has_init&&init&&!map_file_backed(image,init,1u,&file_check))
  return RE_GUEST_ELF32_DYNAMIC_E_RANGE;
 if(has_fini&&fini&&!map_file_backed(image,fini,1u,&file_check))
  return RE_GUEST_ELF32_DYNAMIC_E_RANGE;
 if(has_ia&&iasz&&!map_file_backed(image,ia,iasz,&file_check))
  return RE_GUEST_ELF32_DYNAMIC_E_RANGE;
 if(has_fa&&fasz&&!map_file_backed(image,fa,fasz,&file_check))
  return RE_GUEST_ELF32_DYNAMIC_E_RANGE;
 dynamic->has_init=has_init;dynamic->init_address=init;
 dynamic->has_fini=has_fini;dynamic->fini_address=fini;
 dynamic->has_init_array=has_ia;dynamic->init_array_address=ia;
 dynamic->has_init_array_size=has_iasz;dynamic->init_array_size=iasz;
 dynamic->has_fini_array=has_fa;dynamic->fini_array_address=fa;
 dynamic->has_fini_array_size=has_fasz;dynamic->fini_array_size=fasz;
 if(has_strtab||has_strsz||dynamic->needed_count||has_soname){
  if(!has_strtab||!has_strsz||!strsz||
     !map_file_backed(image,strtab,strsz,&strtab_file)||
     (uint64_t)strtab_file+strsz>size)
   return RE_GUEST_ELF32_DYNAMIC_E_STRING_TABLE;
  dynamic->has_string_table=1u;
  dynamic->string_table_virtual_address=strtab;
  dynamic->string_table_size=strsz;
  dynamic->string_table_file_offset=strtab_file;
  for(size_t i=0;i<dynamic->needed_count;i++){
   REGUESTELF32DYNAMICStatus status=copy_string(bytes,size,strtab_file,strsz,
      needed_offsets[i],dynamic->needed[i]);
   if(status!=RE_GUEST_ELF32_DYNAMIC_OK){if(failed_entry)*failed_entry=i;return status;}
  }
  if(has_soname){
   REGUESTELF32DYNAMICStatus status=copy_string(bytes,size,strtab_file,strsz,
      soname_offset,dynamic->soname);
   if(status!=RE_GUEST_ELF32_DYNAMIC_OK)return status;
   dynamic->has_soname=1u;
  }
 }
 return RE_GUEST_ELF32_DYNAMIC_OK;
}
