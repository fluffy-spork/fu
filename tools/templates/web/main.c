#include "sqlite3.h"

#include "fields.h"
#include "endpoints.h"

#include "fu/app_fu.h"

#include "fu/html_fu.h"
#include "fu/web_fu.h"

#include "resources.h"

int
home_handler(endpoint_t * ep, request_t * req)
{
    if (require_session_web(req, true)) return 0;

    blob_t * html = req->body;

    html_response(req);

    page_begin(ep->title);

    h1(res.hello_world);

    label(B("session id: "), B("session-id"));
    add_s64_blob(html, req->session_id);

    start_div_class(endpoints.hello->name);
    link(endpoints.hello->path, endpoints.hello->title);
    end_div();

    page_end();

    return 0;
}

int
hello_handler(endpoint_t * ep, request_t * req)
{
    param_t * msg = param_endpoint(ep, msg_field);

    if (post_request(req)) {
        post_params(req, ep);

        if (valid_blob(msg->value)) {
            s64 hello_id = 0;
            if (insert_fields_db(app.db, res.hello_table, &hello_id, msg_field, msg->value)) {
                return internal_server_error_response(req);
            }
        }
        else {
            return bad_request_response(req);
        }

        redirect_endpoint(req, ep, NULL); 

        return 0;
    }

    blob_t * html = req->body;

    html_response(req);

    page_begin(ep->title);

    h1(ep->title);

    start_post_form(ep->path, off_autocomplete);
    param_input(msg, 1);
    submit_input(res_html.button, B("submit"));
    end_form();

    int row_handler_func(row_handler_db_t * handler)
    {
        blob_t * html = handler->data;

        start_div_class(B("hello"));

        for (int i = 0; i < handler->n_outputs; i++) {
            param_t * output = &handler->outputs[i];

            start_div_class(output->field->name);
            escape_html(html, output->value);
            end_div();
        }

        end_div();

        return 0;
    }

    row_handler_db_t handler = {
        .func = row_handler_func,
        .data = html
    };

    h1(B("latest"));

    rows_db(app.db, B("select msg, datetime(created, 'unixepoch') as created from hello order by created desc limit 10"), &handler);

    page_end();

    return 0;
}

int
worker_after_fork()
{
    return 0;
}

int
upgrade(const blob_t * db_file)
{
    version_sql_t versions[] = {
        { 1, "create table hello (hello_id integer primary key autoincrement, msg text not null default '', created integer not null default (unixepoch())) strict" }
    };

    return upgrade_db(db_file, versions);
}

int
main(int argc, char *argv[])
{
    if (init_app_fu(argc, argv, stderr_flush_log)) {
        error_log("failed to init app", "app", 1);
        exit(EXIT_FAILURE);
    }

    blob_t * upload_dir = new_path_file_fu(app.state_dir, B("uploads"));
    //debug_blob(upload_dir);

    config_html_t config_html = {
        .max_size_upload = MAX_SIZE_FILE_UPLOAD,
    };

    init_html(&config_html);

    init_res();
    init_fields();

    upgrade(app.main_db_file);

    config_web_t config_web = {
        .port = 8080,
        .res_dir = app.dir,
        .upload_dir = upload_dir,
        .n_workers = 100,
        .n_dev_workers = 10,
        .after_fork_f = worker_after_fork
    };

    return main_web(&config_web);
}

