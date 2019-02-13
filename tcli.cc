#include "app_client.h"

#include <boost/asio.hpp>

int main(int argc, char *argv[])
{
    boost::asio::io_context ioc;

    auto buf = std::make_shared<OutputBuffer>("test");
    strcpy(buf->data(), "hello world\n");
    buf->size(strlen(buf->data()) + 1);

    auto ac = std::make_shared<AppClient>(ioc, "https://p3.theseed.org/services/app_service");
    ac->write_block(buf);

    auto  ac2 = std::make_shared<AppClient>(ioc, "http://beech.mcs.anl.gov:7124");
    ac2->write_block(buf);

    for (int i = 0; i < 20; i++)
    {
	char x[10];
	sprintf(x, "%d", i);
	auto buf = std::make_shared<OutputBuffer>("test");
//	auto buf2 = std::make_shared<OutputBuffer>("test2");
	strcpy(buf->data(), x);
	buf->size(strlen(x) + 1);
	//strcpy(buf2->data(), x);
	//buf2->size(strlen(x) + 1);
	ac2->write_block(buf);
	ac->write_block(buf);
    }
    

    ioc.run();
    std::cerr << "run done\n";
    return 0;
}
