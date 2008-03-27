#ifdef STANDARD
	/* STANDARD is defined, don't use any mysql functions */
	#include <stdlib.h>
	#include <stdio.h>
	#include <string.h>
	#ifdef __WIN__
		typedef unsigned __int64 ulonglong;     /* Microsofts 64 bit types */
		typedef __int64 longlong;
	#else
		typedef unsigned long long ulonglong;
		typedef long long longlong;
	#endif /*__WIN__*/
#else
	#include <my_global.h>
	#include <my_sys.h>
	#include <m_string.h>           /* To get strmov() */
#endif
#include <ruby.h>
#include <mysql.h>
#include <m_ctype.h>
#include <m_string.h>
#include <dirent.h>

// INIT RUBY VM
int VM_STARTED = 0;
const char* script_dir = "/opt/local/share/mysql5/rubyudf/";
void scan_for_vm(); 
void init_ruby_vm(){
	if(VM_STARTED!=1){
		scan_for_vm();
		VM_STARTED = 1;
	}
}

void scan_for_vm(){
	ruby_init();
	ruby_init_loadpath();
	int script_dir_len = strlen(script_dir);
	DIR* scriptdir = opendir(script_dir);
	if(scriptdir != NULL){
		struct dirent *ep;
		while((ep = readdir(scriptdir))){
			if(ep->d_name != "." && ep->d_name != ".."){
				char* fname = (char*) malloc((size_t)sizeof(char) * (script_dir_len + strlen(ep->d_name)));
				strcpy(fname, script_dir);
				strcat(fname, ep->d_name);
				FILE* fp = fopen(fname, "r");
				if(fp!=NULL){
					long size = 0;
					fseek(fp, 0, SEEK_END);
					size = ftell(fp);
					fseek(fp, 0, SEEK_SET);
					char* data = (char*) malloc((size_t)sizeof(char) * size);
					fread(data, 1, size, fp);
					fclose(fp);
					int err;
					rb_eval_string_protect(data, &err);
					if(err){
						VALUE exception = rb_gv_get("$!");
					}
					free(data);
				}
				free(fname);
			}
		}
		closedir(scriptdir);
	}
}

// INIT MYSQL UDFS
#ifdef HAVE_DLOPEN
extern "C" {
	my_bool rb_as_string_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
	char *rb_as_string(UDF_INIT *initid, UDF_ARGS *args,char *result, unsigned long *length,char *is_null, char *error);
}
// rb_as_string
my_bool rb_as_string_init(UDF_INIT *initid, UDF_ARGS *args, char *message){
	if(!args->arg_count >=1){
		strcpy(message, "You need to at least specify a function name.");
		return 1;
	} else {
		scan_for_vm();
		if(!rb_respond_to(rb_cObject, rb_intern(args->args[0]))){
			strcpy(message, "The method specified does not exist.");
			return 1;
		} else {
			return 0;
		}
	}
}

VALUE make_args(UDF_ARGS *args){
	VALUE ary = rb_ary_new();
	for(int i=0;i<args->arg_count ;i++){
		switch(args->arg_type[i]){
			case STRING_RESULT:
				rb_ary_push(ary, rb_str_new2(args->args[i])); 
				break;
			case INT_RESULT:
				longlong lv = *((longlong*) args->args[i]);
				rb_ary_push(ary, INT2NUM(lv));
				break;
			case REAL_RESULT:
				double dv = *((double*) args->args[i]);
				rb_ary_push(ary, rb_float_new(dv));
				break;
			default:
				return NULL;
				break;
		}	
	}
	return ary;
}

static VALUE call_real_func(VALUE args){
	VALUE method = rb_ary_delete_at(args, 0);
	char* rname = RSTRING(rb_obj_as_string(method))->ptr;
	VALUE res = rb_apply(rb_cObject, rb_intern(rname), args);
	return res;
}


char *rb_as_string(UDF_INIT *initid, UDF_ARGS *args,
          char *result, unsigned long *length,
          char *is_null, char *error){

	*is_null = 0;
	int state = 0;
	VALUE rbargs = make_args(args);
	char* oresult = result;
	if(rbargs==NULL){
		*error = 1;	
	} else {
		VALUE res = rb_protect(call_real_func, rbargs, &state);
		if(state!=0){
			*error = 1;
		} else {
			switch(TYPE(res)){
				case T_NIL:
					*is_null = 1;
					break;
				default:
					//free(result);
					char* nresult =  RSTRING(rb_obj_as_string(res))->ptr;
					int dlen = strlen(nresult);
					//result = (char*) malloc((size_t) sizeof(char) * dlen);
					strcpy(result, nresult);
					*length = dlen;
					break;
			}
		}
	}
	return oresult;
}



#endif
