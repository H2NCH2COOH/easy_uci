#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

#include <uci.h>

#include "easy_uci.h"

static struct uci_context* ctx=NULL;

#define ERR_MSG_BUFF_SIZE 256

#define LogE(s) fprintf(stderr,"[%s] %s\n",__func__,s)

#define uci_foreach_element_reverse(_list, _ptr) \
	for(_ptr = list_to_element((_list)->prev); \
		&_ptr->list != (_list); \
		_ptr = list_to_element(_ptr->list.prev))

void easy_uci_free_list(easy_uci_list* list_p)
{
    size_t i;

    for(i=0;i<list_p->len;++i)
    {
        free((char*)list_p->list[i]);
    }
    free(list_p->list);
    list_p->list=NULL;
    list_p->len=0;
}

int easy_uci_get_section_type(const char* package,const char* section,char* buff,size_t size)
{
    int ret;
    struct uci_package* pkg;
    struct uci_section* sec;
    char* err_str=NULL;
    char err_msg[ERR_MSG_BUFF_SIZE];

    if(ctx==NULL)
    {
        ctx=uci_alloc_context();
        if(ctx==NULL)
        {
            snprintf(err_msg,sizeof(err_msg),"Failed to alloc uci context");
            LogE(err_msg);
            return -1;
        }
    }

    ret=uci_load(ctx,package,&pkg);
    if(ret!=0||pkg==NULL)
    {
        snprintf(err_msg,sizeof(err_msg),"Failed to load package: '%s' with error",package);
        goto error_pkg;
    }

    sec=uci_lookup_section(ctx,pkg,section);
    if(sec==NULL)
    {
        snprintf(err_msg,sizeof(err_msg),"Failed to find section: '%s'",section);
        goto error_msg;
    }

    strncpy(buff,sec->type,size);

    uci_unload(ctx,pkg);

    return 0;

/*error_uci:
    uci_unload(ctx,pkg);*/
error_pkg:
    uci_get_errorstr(ctx,&err_str,err_msg);
    LogE(err_str);
    free(err_str);
    return -1;
error_msg:
    uci_unload(ctx,pkg);
    LogE(err_msg);
    return -1;
}

int easy_uci_add_section(const char* package,const char* type,const char* name)
{
    int ret;
    struct uci_package* pkg;
    struct uci_section* sec;
    struct uci_ptr ptr;
    char* err_str=NULL;
    char err_msg[ERR_MSG_BUFF_SIZE];

    if(ctx==NULL)
    {
        ctx=uci_alloc_context();
        if(ctx==NULL)
        {
            snprintf(err_msg,sizeof(err_msg),"Failed to alloc uci context");
            LogE(err_msg);
            return -1;
        }
    }

    ret=uci_load(ctx,package,&pkg);
    if(ret!=0||pkg==NULL)
    {
        snprintf(err_msg,sizeof(err_msg),"Failed to load package: '%s' with error",package);
        goto error_pkg;
    }

    if(name!=NULL&&name[0]!='\0')
    {
        sec=uci_lookup_section(ctx,pkg,name);
        if(sec!=NULL)
        {
            if(strcmp(type,sec->type)!=0)
            {
                snprintf(err_msg,sizeof(err_msg),
                    "Failed to create section: '%s' of type: '%s' because a different section with the same name and type: '%s' already exists",
                    name,type,sec->type);
                goto error_msg;
            }

            uci_unload(ctx,pkg);

            return 0;
        }

        //Add named section
        memset(&ptr,0,sizeof(struct uci_ptr));
        ptr.package=package;
        ptr.section=name;
        ptr.value=type;
        ptr.p=pkg;

        ret=uci_set(ctx,&ptr);
        if(ret!=0)
        {
            snprintf(err_msg,sizeof(err_msg),"Failed to create section: '%s' of type: '%s' with error",name,type);
            goto error_uci;
        }
    }
    else
    {
        //Add anonymous section
        ret=uci_add_section(ctx,pkg,type,&sec);
        if(ret!=0)
        {
            snprintf(err_msg,sizeof(err_msg),"Failed to create anonymous section of type: '%s' with error",type);
            goto error_uci;
        }
    }

    uci_save(ctx,pkg);
    uci_commit(ctx,&pkg,false);
    uci_unload(ctx,pkg);

    return 0;

error_uci:
    uci_unload(ctx,pkg);
error_pkg:
    uci_get_errorstr(ctx,&err_str,err_msg);
    LogE(err_str);
    free(err_str);
    return -1;
error_msg:
    uci_unload(ctx,pkg);
    LogE(err_msg);
    return -1;
}

int easy_uci_delete_section(const char* package,const char* section)
{
    int ret;
    struct uci_package* pkg;
    struct uci_section* sec;
    struct uci_ptr ptr;
    char* err_str=NULL;
    char err_msg[ERR_MSG_BUFF_SIZE];

    if(ctx==NULL)
    {
        ctx=uci_alloc_context();
        if(ctx==NULL)
        {
            snprintf(err_msg,sizeof(err_msg),"Failed to alloc uci context");
            LogE(err_msg);
            return -1;
        }
    }

    ret=uci_load(ctx,package,&pkg);
    if(ret!=0||pkg==NULL)
    {
        snprintf(err_msg,sizeof(err_msg),"Failed to load package: '%s' with error",package);
        goto error_pkg;
    }

    sec=uci_lookup_section(ctx,pkg,section);
    if(sec!=NULL)
    {
        memset(&ptr,0,sizeof(struct uci_ptr));
        ptr.package=package;
        ptr.section=section;
        ptr.p=pkg;
        ptr.s=sec;

        ret=uci_delete(ctx,&ptr);
        if(ret!=0)
        {
            snprintf(err_msg,sizeof(err_msg),"Failed to delete section: '%s' with error",section);
            goto error_uci;
        }

        uci_save(ctx,pkg);
        uci_commit(ctx,&pkg,false);
    }

    uci_unload(ctx,pkg);

    return 0;

error_uci:
    uci_unload(ctx,pkg);
error_pkg:
    uci_get_errorstr(ctx,&err_str,err_msg);
    LogE(err_str);
    free(err_str);
    return -1;
/*error_msg:
    uci_unload(ctx,pkg);
    LogE(err_msg);
    return -1;*/
}

int easy_uci_get_all_section_of_type(const char* package,const char* type,easy_uci_list* list_p)
{
    int ret,i;
    struct uci_package* pkg;
    struct uci_section* sec;
    struct uci_section* list[1024];
    struct uci_element* e;
    int sec_count=0;
    char** ss=NULL;
    char* err_str=NULL;
    char err_msg[ERR_MSG_BUFF_SIZE];

    if(ctx==NULL)
    {
        ctx=uci_alloc_context();
        if(ctx==NULL)
        {
            snprintf(err_msg,sizeof(err_msg),"Failed to alloc uci context");
            LogE(err_msg);
            return -1;
        }
    }

    ret=uci_load(ctx,package,&pkg);
    if(ret!=0||pkg==NULL)
    {
        snprintf(err_msg,sizeof(err_msg),"Failed to load package: '%s' with error",package);
        goto error_pkg;
    }

    uci_foreach_element(&pkg->sections,e)
    {
        sec=uci_to_section(e);
        if(strcmp(sec->type,type)==0)
        {
            list[sec_count++]=sec;
        }
    }

    if(sec_count>0)
    {
        ss=malloc(sizeof(char*)*sec_count);
        if(ss==NULL)
        {
            snprintf(err_msg,sizeof(err_msg),"Failed malloc at %s:%d",__FILE__,__LINE__);
            goto error_msg;
        }

        for(i=0;i<sec_count;++i)
        {
            ss[i]=strdup(list[i]->e.name);
            if(ss[i]==NULL)
            {
                for(--i;i>=0;--i)
                {
                    free(ss[i]);
                }
                free(ss);
                snprintf(err_msg,sizeof(err_msg),"Failed malloc at %s:%d",__FILE__,__LINE__);
                goto error_msg;
            }
        }

        list_p->list=(const char**)ss;
        list_p->len=sec_count;
    }
    else
    {
        list_p->list=NULL;
        list_p->len=0;
    }

    uci_unload(ctx,pkg);

    return 0;

/*error_uci:
    uci_unload(ctx,pkg);*/
error_pkg:
    uci_get_errorstr(ctx,&err_str,err_msg);
    LogE(err_str);
    free(err_str);
    return -1;
error_msg:
    uci_unload(ctx,pkg);
    LogE(err_msg);
    return -1;
}

int easy_uci_get_nth_section_of_type(const char* package,const char* type,int n,char** name_p)
{
    int ret,i;
    struct uci_package* pkg;
    struct uci_section* sec;
    struct uci_element* e;
    bool found=false;
    char* name;
    char* err_str=NULL;
    char err_msg[ERR_MSG_BUFF_SIZE];
    
    if(ctx==NULL)
    {
        ctx=uci_alloc_context();
        if(ctx==NULL)
        {
            snprintf(err_msg,sizeof(err_msg),"Failed to alloc uci context");
            LogE(err_msg);
            return -1;
        }
    }
    
    ret=uci_load(ctx,package,&pkg);
    if(ret!=0||pkg==NULL)
    {
        snprintf(err_msg,sizeof(err_msg),"Failed to load package: '%s' with error",package);
        goto error_pkg;
    }
    
    i=n;
    if(i>=0)
    {
        uci_foreach_element(&pkg->sections,e)
        {
            sec=uci_to_section(e);
            if(strcmp(sec->type,type)==0)
            {
                if(i==0)
                {
                    found=true;
                    break;
                }
                else
                {
                    --i;
                }
            }
        }
    }
    else
    {
        uci_foreach_element_reverse(&pkg->sections,e)
        {
            sec=uci_to_section(e);
            if(strcmp(sec->type,type)==0)
            {
                if(i==-1)
                {
                    found=true;
                    break;
                }
                else
                {
                    ++i;
                }
            }
        }
    }
    
    if(!found)
    {
        snprintf(err_msg,sizeof(err_msg),"Can't find section of type '%s' at index %d",type,n);
        goto error_msg;
    }
    
    name=strdup(sec->e.name);
    if(name==NULL)
    {
        snprintf(err_msg,sizeof(err_msg),"Failed malloc at %s:%d",__FILE__,__LINE__);
        goto error_msg;
    }
    
    *name_p=name;
    
    uci_unload(ctx,pkg);

    return 0;

error_pkg:
    uci_get_errorstr(ctx,&err_str,err_msg);
    LogE(err_str);
    free(err_str);
    return -1;
error_msg:
    uci_unload(ctx,pkg);
    LogE(err_msg);
    return -1;
}

int easy_uci_get_option_string(const char* package,const char* section,const char* option,char* buff,size_t size)
{
    int ret;
    struct uci_package* pkg;
    struct uci_section* sec;
    struct uci_option*  opt;
    char* err_str=NULL;
    char err_msg[ERR_MSG_BUFF_SIZE];

    if(ctx==NULL)
    {
        ctx=uci_alloc_context();
        if(ctx==NULL)
        {
            snprintf(err_msg,sizeof(err_msg),"Failed to alloc uci context");
            LogE(err_msg);
            return -1;
        }
    }

    ret=uci_load(ctx,package,&pkg);
    if(ret!=0||pkg==NULL)
    {
        snprintf(err_msg,sizeof(err_msg),"Failed to load package: '%s' with error",package);
        goto error_pkg;
    }

    sec=uci_lookup_section(ctx,pkg,section);
    if(sec==NULL)
    {
        snprintf(err_msg,sizeof(err_msg),"Failed to find section: '%s'",section);
        goto error_msg;
    }

    opt=uci_lookup_option(ctx,sec,option);
    if(opt==NULL)
    {
        snprintf(err_msg,sizeof(err_msg),"Failed to find option: '%s'",option);
        goto error_msg;
    }

    if(!opt||opt->type!=UCI_TYPE_STRING)
    {
        snprintf(err_msg,sizeof(err_msg),"Option: '%s' is not a string",option);
        goto error_msg;
    }

    strncpy(buff,opt->v.string,size);

    uci_unload(ctx,pkg);

    return 0;

/*error_uci:
    uci_unload(ctx,pkg);*/
error_pkg:
    uci_get_errorstr(ctx,&err_str,err_msg);
    LogE(err_str);
    free(err_str);
    return -1;
error_msg:
    uci_unload(ctx,pkg);
    LogE(err_msg);
    return -1;
}

int easy_uci_set_option_string(const char* package,const char* section,const char* option,const char* value)
{
    int ret;
    struct uci_package* pkg;
    struct uci_section* sec;
    struct uci_ptr ptr;
    char* err_str=NULL;
    char err_msg[ERR_MSG_BUFF_SIZE];

    if(value==NULL||value[0]=='\0')
    {
        return -1;
    }

    if(ctx==NULL)
    {
        ctx=uci_alloc_context();
        if(ctx==NULL)
        {
            snprintf(err_msg,sizeof(err_msg),"Failed to alloc uci context");
            LogE(err_msg);
            return -1;
        }
    }

    ret=uci_load(ctx,package,&pkg);
    if(ret!=0||pkg==NULL)
    {
        snprintf(err_msg,sizeof(err_msg),"Failed to load package: '%s' with error",package);
        goto error_pkg;
    }

    sec=uci_lookup_section(ctx,pkg,section);
    if(sec==NULL)
    {
        snprintf(err_msg,sizeof(err_msg),"Failed to find section: '%s'",section);
        goto error_msg;
    }

    memset(&ptr,0,sizeof(struct uci_ptr));

    ptr.package=package;
    ptr.section=section;
    ptr.option=option;
    ptr.value=value;

    ptr.p=pkg;
    ptr.s=sec;

    ret=uci_set(ctx,&ptr);
    if(ret!=0)
    {
        snprintf(err_msg,sizeof(err_msg),"Failed to set option: '%s' with error",option);
        goto error_uci;
    }

    uci_save(ctx,pkg);
    uci_commit(ctx,&pkg,false);
    uci_unload(ctx,pkg);

    return 0;

error_uci:
    uci_unload(ctx,pkg);
error_pkg:
    uci_get_errorstr(ctx,&err_str,err_msg);
    LogE(err_str);
    free(err_str);
    return -1;
error_msg:
    uci_unload(ctx,pkg);
    LogE(err_msg);
    return -1;
}

int easy_uci_get_option_list(const char* package,const char* section,const char* option,easy_uci_list* list_p)
{
    int ret;
    struct uci_package* pkg;
    struct uci_section* sec;
    struct uci_option*  opt;
    struct uci_element* e;
    char** ss;
    int count=0;
    char* err_str=NULL;
    char err_msg[ERR_MSG_BUFF_SIZE];

    if(ctx==NULL)
    {
        ctx=uci_alloc_context();
        if(ctx==NULL)
        {
            snprintf(err_msg,sizeof(err_msg),"Failed to alloc uci context");
            LogE(err_msg);
            return -1;
        }
    }

    ret=uci_load(ctx,package,&pkg);
    if(ret!=0||pkg==NULL)
    {
        snprintf(err_msg,sizeof(err_msg),"Failed to load package: '%s' with error",package);
        goto error_pkg;
    }

    sec=uci_lookup_section(ctx,pkg,section);
    if(sec==NULL)
    {
        snprintf(err_msg,sizeof(err_msg),"Failed to find section: '%s'",section);
        goto error_msg;
    }

    opt=uci_lookup_option(ctx,sec,option);
    if(opt==NULL)
    {
        snprintf(err_msg,sizeof(err_msg),"Failed to find option: '%s'",option);
        goto error_msg;
    }

    if(!opt||opt->type!=UCI_TYPE_LIST)
    {
        snprintf(err_msg,sizeof(err_msg),"Option: '%s' is not a list",option);
        goto error_msg;
    }

    uci_foreach_element(&opt->v.list,e)
    {
        ++count;
    }
    
    if(count>0)
    {
        ss=malloc(sizeof(char*)*count);
        if(ss==NULL)
        {
            snprintf(err_msg,sizeof(err_msg),"Failed malloc at %s:%d",__FILE__,__LINE__);
            goto error_msg;
        }
        
        count=0;
        uci_foreach_element(&opt->v.list,e)
        {
            ss[count]=strdup(e->name);
            if(ss[count]==NULL)
            {
                LogE(err_msg);
                for(--count;count>=0;--count)
                {
                    free(ss[count]);
                }
                snprintf(err_msg,sizeof(err_msg),"Failed malloc at %s:%d",__FILE__,__LINE__);
                goto error_msg;
            }
            ++count;
        }
        
        list_p->list=(const char**)ss;
        list_p->len=count;
    }
    else
    {
        list_p->list=NULL;
        list_p->len=0;
    }

    uci_unload(ctx,pkg);

    return 0;

/*error_uci:
    uci_unload(ctx,pkg);*/
error_pkg:
    uci_get_errorstr(ctx,&err_str,err_msg);
    LogE(err_str);
    free(err_str);
    return -1;
error_msg:
    uci_unload(ctx,pkg);
    LogE(err_msg);
    return -1;
}

int easy_uci_set_option_list(const char* package,const char* section,const char* option,easy_uci_list* list_p)
{
    int ret;
    size_t i;
    struct uci_package* pkg;
    struct uci_section* sec;
    struct uci_option*  opt;
    struct uci_ptr ptr;
    char* err_str=NULL;
    char err_msg[ERR_MSG_BUFF_SIZE];

    if(list_p==NULL||list_p->len==0||list_p->list==NULL)
    {
        return -1;
    }

    if(ctx==NULL)
    {
        ctx=uci_alloc_context();
        if(ctx==NULL)
        {
            snprintf(err_msg,sizeof(err_msg),"Failed to alloc uci context");
            LogE(err_msg);
            return -1;
        }
    }

    ret=uci_load(ctx,package,&pkg);
    if(ret!=0||pkg==NULL)
    {
        snprintf(err_msg,sizeof(err_msg),"Failed to load package: '%s' with error",package);
        goto error_pkg;
    }

    sec=uci_lookup_section(ctx,pkg,section);
    if(sec==NULL)
    {
        snprintf(err_msg,sizeof(err_msg),"Failed to find section: '%s'",section);
        goto error_msg;
    }

    opt=uci_lookup_option(ctx,sec,option);
    if(opt!=NULL)
    {
        memset(&ptr,0,sizeof(struct uci_ptr));
        ptr.package=package;
        ptr.section=section;
        ptr.option=option;
        ptr.p=pkg;
        ptr.s=sec;
        ptr.o=opt;
        
        ret=uci_delete(ctx,&ptr);
        if(ret!=0)
        {
            snprintf(err_msg,sizeof(err_msg),"Failed to delete old option: '%s' with error",option);
            goto error_uci;
        }
    }
    
    memset(&ptr,0,sizeof(struct uci_ptr));
    ptr.package=package;
    ptr.section=section;
    ptr.option=option;
    ptr.p=pkg;
    ptr.s=sec;
    
    for(i=0;i<list_p->len;++i)
    {
        ptr.value=list_p->list[i];
        ret=uci_add_list(ctx,&ptr);
        if(ret!=0)
        {
            snprintf(err_msg,sizeof(err_msg),"Failed to append to option: '%s' with error",option);
            goto error_uci;
        }
    }

    uci_save(ctx,pkg);
    uci_commit(ctx,&pkg,false);
    uci_unload(ctx,pkg);

    return 0;

error_uci:
    uci_unload(ctx,pkg);
error_pkg:
    uci_get_errorstr(ctx,&err_str,err_msg);
    LogE(err_str);
    free(err_str);
    return -1;
error_msg:
    uci_unload(ctx,pkg);
    LogE(err_msg);
    return -1;
}

int easy_uci_append_to_option_list(const char* package,const char* section,const char* option,const char* value)
{
    int ret;
    struct uci_package* pkg;
    struct uci_section* sec;
    struct uci_ptr ptr;
    char* err_str=NULL;
    char err_msg[ERR_MSG_BUFF_SIZE];

    if(value==NULL)
    {
        return -1;
    }

    if(ctx==NULL)
    {
        ctx=uci_alloc_context();
        if(ctx==NULL)
        {
            snprintf(err_msg,sizeof(err_msg),"Failed to alloc uci context");
            LogE(err_msg);
            return -1;
        }
    }

    ret=uci_load(ctx,package,&pkg);
    if(ret!=0||pkg==NULL)
    {
        snprintf(err_msg,sizeof(err_msg),"Failed to load package: '%s' with error",package);
        goto error_pkg;
    }

    sec=uci_lookup_section(ctx,pkg,section);
    if(sec==NULL)
    {
        snprintf(err_msg,sizeof(err_msg),"Failed to find section: '%s'",section);
        goto error_msg;
    }

    memset(&ptr,0,sizeof(struct uci_ptr));
    ptr.package=package;
    ptr.section=section;
    ptr.option=option;
    ptr.p=pkg;
    ptr.s=sec;
    ptr.value=value;
    ret=uci_add_list(ctx,&ptr);
    if(ret!=0)
    {
        snprintf(err_msg,sizeof(err_msg),"Failed to append to option: '%s' with error",option);
        goto error_uci;
    }

    uci_save(ctx,pkg);
    uci_commit(ctx,&pkg,false);
    uci_unload(ctx,pkg);

    return 0;

error_uci:
    uci_unload(ctx,pkg);
error_pkg:
    uci_get_errorstr(ctx,&err_str,err_msg);
    LogE(err_str);
    free(err_str);
    return -1;
error_msg:
    uci_unload(ctx,pkg);
    LogE(err_msg);
    return -1;
}

int easy_uci_delete_option(const char* package,const char* section,const char* option)
{
    int ret;
    struct uci_package* pkg;
    struct uci_ptr ptr;
    char* err_str=NULL;
    char err_msg[ERR_MSG_BUFF_SIZE];

    if(ctx==NULL)
    {
        ctx=uci_alloc_context();
        if(ctx==NULL)
        {
            snprintf(err_msg,sizeof(err_msg),"Failed to alloc uci context");
            LogE(err_msg);
            return -1;
        }
    }

    ret=uci_load(ctx,package,&pkg);
    if(ret!=0||pkg==NULL)
    {
        snprintf(err_msg,sizeof(err_msg),"Failed to load package: '%s' with error",package);
        goto error_pkg;
    }
    
    memset(&ptr,0,sizeof(struct uci_ptr));

    ptr.package=package;
    ptr.section=section;
    ptr.option=option;

    ret=uci_delete(ctx,&ptr);

    uci_save(ctx,pkg);
    uci_commit(ctx,&pkg,false);
    uci_unload(ctx,pkg);

    return 0;

/*error_uci:
    uci_unload(ctx,pkg);*/
error_pkg:
    uci_get_errorstr(ctx,&err_str,err_msg);
    LogE(err_str);
    free(err_str);
    return -1;
/*error_msg:
    uci_unload(ctx,pkg);
    LogE(err_msg);
    return -1;*/
}
