#include <userver/components/minimal_server_component_list.hpp>
#include <userver/utils/daemon_run.hpp>

#include "api/http_handlers.hpp"

int main(int argc, char* argv[]) {
    auto component_list = userver::components::MinimalServerComponentList();

    mail_lab::api::AppendHandlers(component_list);

    return userver::utils::DaemonMain(argc, argv, component_list);
}
