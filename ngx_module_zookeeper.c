#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <c-client-src/zookeeper.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>

static zhandle_t *zh;
char zk_server[1256];
int try_times = 3;
const char* prefix = "/3g/pics/";

static char* ngx_zookeeper_readconf(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static void* ngx_zookeeper_create_main_conf(ngx_conf_t *cf);
static char *ngx_zookeeper_init_main_conf(ngx_conf_t *cf, void *conf);
static ngx_int_t init_process(ngx_cycle_t *cycle);

typedef struct
{
  ngx_str_t ecdata;
}ngx_zookeeper_main_conf_t;

static ngx_command_t  ngx_zookeeper_commands[] = {
	{ ngx_string("zk_server"),
		NGX_ANY_CONF|NGX_CONF_TAKE1,
		ngx_zookeeper_readconf,
		NGX_HTTP_MAIN_CONF_OFFSET,
		offsetof(ngx_zookeeper_main_conf_t, ecdata),
		NULL },
	ngx_null_command
};

static ngx_http_module_t  ngx_zookeeper_module_ctx = {
	NULL,        /* preconfiguration */
	NULL,             /* postconfiguration */

	ngx_zookeeper_create_main_conf,/* create main configuration */
	ngx_zookeeper_init_main_conf,  /* init main configuration */

	NULL,                          /* create server configuration */
	NULL,                          /* merge server configuration */

	NULL,  /* create location configuration */
	NULL
};

ngx_module_t  ngx_module_zookeeper = {
	NGX_MODULE_V1,
	&ngx_zookeeper_module_ctx, /* module context */
	ngx_zookeeper_commands,   /* module directives */
	NGX_HTTP_MODULE,               /* module type */
	NULL,                          /* init master */
	NULL,                          /* init module */
	init_process,                          /* init process */
	NULL,                          /* init thread */
	NULL,                          /* exit thread */
	NULL,                          /* exit process */
	NULL,                          /* exit master */
	NGX_MODULE_V1_PADDING
};

void default_watcher(zhandle_t *zh, int type, int state, const char *path, void* context)
{

}

void initZkConn(){
	int try = 0;
	zh = zookeeper_init(zk_server, default_watcher, 10000, 0, 0, 0);

	while (zh == NULL||try < try_times) {
		zh = zookeeper_init(zk_server, default_watcher, 10000, 0, 0, 0);
		try++;
	}
	if(zh == NULL){
		fprintf(stderr, "Error when connecting to zookeeper servers...\n");
		exit(-1);
	}
}

void createNode(char* nodeName){

	char node[20];
	memset (node,0,20);
	strcat(node,prefix);
	strcat(node,nodeName);

	int res = zoo_create(zh,node,"alive",5,&ZOO_OPEN_ACL_UNSAFE,ZOO_EPHEMERAL,NULL,0);
	if(res !=ZOK){
		fprintf(stdout,"eror in create \n");
	}else{
		fprintf(stdout,"sucess\n");
	}
}

char* getIpAddress(){

	char hname[128];
	struct hostent *hent;
	gethostname(hname, sizeof(hname));

	hent = gethostbyname(hname);
	char* ip = inet_ntoa(*(struct in_addr*)(hent->h_addr_list[0]));
	return ip;
}


static char *
ngx_zookeeper_readconf(ngx_conf_t *cf, ngx_command_t *cmd, void *conf){
	ngx_http_core_loc_conf_t *clcf;
	ngx_zookeeper_main_conf_t* local_conf;	
	clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
	local_conf = conf;

	ngx_conf_set_str_slot(cf,cmd,conf);
	printf("%s\n",local_conf->ecdata.data);
	memset(zk_server,0,1256);

	sprintf(zk_server,"%s",local_conf->ecdata.data);	
	return NGX_CONF_OK;
}

static void *
ngx_zookeeper_create_main_conf(ngx_conf_t *cf){
	ngx_zookeeper_main_conf_t *conf;
	conf  = ngx_palloc(cf->pool,sizeof(ngx_zookeeper_main_conf_t));
	if (conf == NULL) {
		return NGX_CONF_ERROR;
	}
	conf->ecdata.len = 0;
	conf->ecdata.data = NULL;

	return conf;
}

static char *ngx_zookeeper_init_main_conf(ngx_conf_t *cf, void *conf){

	return NGX_CONF_OK;
}

static ngx_int_t init_process(ngx_cycle_t *cycle){
	char *name = getIpAddress();
	initZkConn();
	createNode(name);	
	return NGX_OK;
}
