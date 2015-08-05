#ifdef __cpluscplus
extern "C"{
#endif
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#ifdef __cpluscplus
}
#endif
#include "ppport.h"
#include "../c++/include/qconf.h"
#include <string.h>


SV* get_conf(const char *path,SV *ret,const char *idc, const char *async)
{
	char *value = new char[1024*1024];	  
	memset(value,0,1024*1024);	  
	char *idc_ = NULL;
	if (strlen(idc) != 0)
		idc_ = (char*)idc;
    int err = 0;
    if(strcasecmp(async,"true") != 0)
    	err = qconf_get_conf(path, value, 1024*1024, idc_);
    else
    	err = qconf_aget_conf(path, value, 1024*1024, idc_);
    SV* errCode = newSViv(err);
    if (err != QCONF_OK)
    	goto final;
    sv_setpvn(ret,value,strlen(value));      
final:      
    delete [] value;
    return errCode;
}

SV* get_batch_conf(const char *path,HV *ret,const char *idc, const char *async)
{
	qconf_batch_nodes nodes;	
	int err = init_qconf_batch_nodes(&nodes);
	if (QCONF_OK != err)
		return newSViv(err);
	char *idc_ = NULL;
	if (strlen(idc) != 0)
		idc_ = (char*)idc;
	if(strcasecmp(async,"true") != 0)
		err = qconf_get_batch_conf(path,&nodes,idc_);
	else
		err = qconf_aget_batch_conf(path,&nodes,idc_);
	if (QCONF_OK != err){
		destroy_qconf_batch_nodes(&nodes);
		return newSViv(err);
	}
	for(int i=0;i<nodes.count;++i)
	{
		hv_store(ret,nodes.nodes[i].key,strlen(nodes.nodes[i].key),newSVpvn(nodes.nodes[i].value,strlen(nodes.nodes[i].value)),0);
	}	
	destroy_qconf_batch_nodes(&nodes);
	return newSViv(err);
}

SV* get_batch_keys(const char *path, AV *ret,const char *idc, const char *async)
{
	string_vector_t nodes;
	int err = init_string_vector(&nodes);
	if(QCONF_OK != err)
		return newSViv(err);
	char *idc_ = NULL;
	if (strlen(idc) != 0)
		idc_ = (char*)idc;
	if(strcasecmp(async,"true") != 0)
		err = qconf_get_batch_keys(path,&nodes,idc_);
	else
		err = qconf_aget_batch_keys(path,&nodes,idc_);
	if(QCONF_OK != err)
	{
		destroy_string_vector(&nodes);
		return newSViv(err);
	}
	for(int i = 0;i < nodes.count; ++i)
	{
		av_push(ret,newSVpvn(nodes.data[i],strlen(nodes.data[i])));
	}
	destroy_string_vector(&nodes);
	return newSViv(err);	
}

SV* get_batch_keys_native(const char *path, AV *ret,const char *idc, const char *async)
{
	string_vector_t nodes;
	int err = init_string_vector(&nodes);
	if(QCONF_OK != err)
		return newSViv(err);
	char *idc_ = NULL;
	if(strcasecmp(async,"true") != 0)
		idc_ = (char*)idc;
	if(!async)
		err = qconf_get_batch_keys_native(path,&nodes,idc_);
	else
		err = qconf_aget_batch_keys_native(path,&nodes,idc_);
	if(QCONF_OK != err)
	{
		destroy_string_vector(&nodes);
		return newSViv(err);
	}
	for(int i = 0;i < nodes.count; ++i)
	{
		av_push(ret,newSVpvn(nodes.data[i],strlen(nodes.data[i])));
	}
	destroy_string_vector(&nodes);
	return newSViv(err);	
}

SV* get_host(const char *path,SV *ret,const char *idc, const char *async)
{
	char *value = new char[1024*1024];	  
	memset(value,0,1024*1024);	  
	char *idc_ = NULL;
	if (strlen(idc) != 0)
		idc_ = (char*)idc;
    int err = 0;
    if(strcasecmp(async,"true") != 0)
    	err = qconf_get_host(path, value, 1024*1024, idc_);
    else
    	err = qconf_aget_host(path, value, 1024*1024, idc_);
    SV* errCode = newSViv(err);
    if (err != QCONF_OK)
    	goto final;
    sv_setpvn(ret,value,strlen(value));      
final:      
    delete [] value;
    return errCode;
}

SV* get_allhost(const char *path, AV *ret,const char *idc, const char *async)
{
	string_vector_t nodes;
	int err = init_string_vector(&nodes);
	if(QCONF_OK != err)
		return newSViv(err);
	char *idc_ = NULL;
	if (strlen(idc) != 0)
		idc_ = (char*)idc;
	if(strcasecmp(async,"true") != 0)
		err = qconf_get_allhost(path,&nodes,idc_);
	else
		err = qconf_aget_allhost(path,&nodes,idc_);
	if(QCONF_OK != err)
	{
		destroy_string_vector(&nodes);
		return newSViv(err);
	}
	for(int i = 0;i < nodes.count; ++i)
	{
		av_push(ret,newSVpvn(nodes.data[i],strlen(nodes.data[i])));
	}
	destroy_string_vector(&nodes);
	return newSViv(err);
}


MODULE = QConf		PACKAGE = QConf		

int
qconf_init()

int
qconf_destroy()

SV*
get_conf(const char *path,SV *ret,const char *idc, const char *async)

SV*
get_batch_conf(const char *path,HV *ret,const char *idc, const char *async)

SV*
get_batch_keys(const char *path, AV *ret,const char *idc, const char *async)

SV*
get_batch_keys_native(const char *path, AV *ret,const char *idc, const char *async)

SV*
get_host(const char *path,SV *ret,const char *idc, const char *async)

SV*
get_allhost(const char *path, AV *ret, const char *idc, const char *async)
