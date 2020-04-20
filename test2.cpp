//
// Created by oil_you on 2019/11/4.
//书本P302

void newConnection(int sockfd, const InetAddress& peerAddr)
{
    printf("newConnection() : accepted a new connection from %s\n",
           peerAddr.getSockAddr()->sa_data);
    ::write(sockfd, "How are you?\n", 13);
    close(sockfd);
}

int main()
{
    printf("main(): pid = %d\n", getpid());

    InetAddress listenAddr(8888);
    EventLoop loop;

    Acceptor acceptor(&loop, listenAddr, true);
    acceptor.setNewConnectionCallback(newConnection);
    acceptor.listen();

    loop.loop();
}