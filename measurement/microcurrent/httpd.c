/*
 * Microcurrent httpd.
 *
 * Authors:
 *  Antti Partanen <aehparta@iki.fi>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <microhttpd.h>
#include "httpd.h"
#include "ldo.h"


static struct MHD_Daemon *daemon_handle;
static char *www_root = NULL;


static int httpd_404(struct MHD_Connection *connection, int code)
{
	int err;
	struct MHD_Response *response;
	static char *data = "<html><head><title>404</title></head><body><h1>404</h1></body></html>";
	response = MHD_create_response_from_buffer(strlen(data),
	           (void *)data, MHD_RESPMEM_PERSISTENT);
	MHD_add_response_header(response, "Content-Type", "text/html; charset=utf-8");
	err = MHD_queue_response(connection, code, response);
	MHD_destroy_response(response);
	return err;
}

static int httpd_get_default(struct MHD_Connection *connection, const char *url)
{
	int err = 0;
	struct MHD_Response *response;
	char *file = NULL;
	int fd = -1;
	const char *ext = NULL;
	struct stat st;

	if (strlen(url) == 1) {
		url = "/index.html";
	}
	ext = url + strlen(url) - 1;
	for (; ext > url && *ext != '.' ; ext--);
	ext++;

	/* check filesystem */
	if (asprintf(&file, "%s%s", www_root, url) < 0) {
		return httpd_404(connection, MHD_HTTP_NOT_FOUND);
	}
	fd = open(file, O_RDONLY);
	free(file);
	if (fd < 0) {
		return httpd_404(connection, MHD_HTTP_NOT_FOUND);
	}

	/* read file size */
	fstat(fd, &st);

	response = MHD_create_response_from_fd(st.st_size, fd);
	if (strcmp(ext, "css") == 0) {
		MHD_add_response_header(response, "Content-Type", "text/css; charset=utf-8");
	} else 	if (strcmp(ext, "js") == 0) {
		MHD_add_response_header(response, "Content-Type", "application/javascript; charset=utf-8");
	} else 	if (strcmp(ext, "json") == 0) {
		MHD_add_response_header(response, "Content-Type", "application/json; charset=utf-8");
	} else {
		MHD_add_response_header(response, "Content-Type", "text/html; charset=utf-8");
	}
	err = MHD_queue_response(connection, MHD_HTTP_OK, response);
	MHD_destroy_response(response);

	return err;
}

static int httpd_voltage_get(struct MHD_Connection *connection)
{
	int err;
	struct MHD_Response *response;
	char data[32];
	const char *v = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "v");

	/* set voltage if given as query parameter */
	if (v) {
		char *p = NULL;
		float voltage = strtof(v, &p);
		if (v != p) {
			ldo_voltage(voltage);
		}
	}

	/* return current voltage */
	sprintf(data, "%0.3f", ldo_voltage(NAN));
	response = MHD_create_response_from_buffer(strlen(data), (void *)data, MHD_RESPMEM_MUST_COPY);
	MHD_add_response_header(response, "Content-Type", "text/plain");
	err = MHD_queue_response(connection, MHD_HTTP_OK, response);
	MHD_destroy_response(response);

	return err;
}

static int httpd_request_handler(void *cls, struct MHD_Connection *connection,
                                 const char *url,
                                 const char *method, const char *version,
                                 const char *upload_data,
                                 size_t *upload_data_size, void **con_cls)
{
	/* only GET can get through here */
	if (strcmp(method, "GET") != 0) {
		return httpd_404(connection, MHD_HTTP_NOT_FOUND);
	}

	/* ldo voltage set/get */
	if (strcmp(url, "/voltage") == 0) {
		return httpd_voltage_get(connection);
	}

	/* default GET handler */
	return httpd_get_default(connection, url);
}

int httpd_init(void)
{
	/* just copied this code from another project, init not really needed here, but for reasons unknown I simply cannot remove it, might need it later?!?!? */
	return 0;
}

void httpd_quit(void)
{
	if (daemon_handle) {
		MHD_stop_daemon(daemon_handle);
		daemon_handle = NULL;
	}
	if (www_root) {
		free(www_root);
		www_root = NULL;
	}
}

int httpd_start(char *address, uint16_t port, char *root)
{
	www_root = strdup(root);
	daemon_handle = MHD_start_daemon(MHD_USE_EPOLL_INTERNAL_THREAD | MHD_USE_ERROR_LOG | MHD_ALLOW_UPGRADE,
	                                 port, NULL, NULL,
	                                 &httpd_request_handler, NULL, MHD_OPTION_END);
	if (!daemon_handle) {
		return -1;
	}

	return 0;
}
