#include <boost/shared_ptr.hpp>
#include <memory>

#include "rtmp_server.hpp"
#include "rtmp_server_session.hpp"
namespace mms {
    // connÊôÓÚserver,sessionÊôÓÚconn
    void RtmpServer::on_tcp_socket_open(std::shared_ptr<TcpSocket> conn) {
        auto rtmp_conn = std::static_pointer_cast<RtmpConn>(conn);
        std::shared_ptr<RtmpServerSession> s = std::make_shared<RtmpServerSession>(rtmp_conn);
        rtmp_conn->set_session(s);
        spdlog::info("RtmpServer Session create");
        s->service();
    }

    void RtmpServer::on_tcp_socket_close(std::shared_ptr<TcpSocket> conn) {
        auto rtmp_conn = std::static_pointer_cast<RtmpConn>(conn);
        std::shared_ptr<RtmpServerSession> s = std::static_pointer_cast<RtmpServerSession>(rtmp_conn->get_session());
        rtmp_conn->clear_session();
        if (s) {
            s->close();
            spdlog::info("RtmpServer Session close");
        }

    }

};