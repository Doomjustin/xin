#include <cstdlib>

import Log;


int main(int argc, char* argv[])
{
    const auto logger = xin::log::Logger::instance();
    logger->trace("test message: {}", 1);
    logger->debug("test message: {}", 2);

    return EXIT_SUCCESS;
}
