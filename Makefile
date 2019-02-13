TOP_DIR = ../..
include $(TOP_DIR)/tools/Makefile.common

TARGET ?= /kb/deployment
DEPLOY_TARGET ?= $(TARGET)
DEPLOY_RUNTIME ?= /disks/patric-common/runtime

SRC_SERVICE_PERL = $(wildcard service-scripts/*.pl)
BIN_SERVICE_PERL = $(addprefix $(BIN_DIR)/,$(basename $(notdir $(SRC_SERVICE_PERL))))
DEPLOY_SERVICE_PERL = $(addprefix $(SERVICE_DIR)/bin/,$(basename $(notdir $(SRC_SERVICE_PERL))))

STARMAN_WORKERS = 5

#DATA_API_URL = https://www.patricbrc.org/api
DATA_API_URL = https://p3.theseed.org/services/data_api

BUILD_TOOLS = $(DEPLOY_RUNTIME)/gcc-4.9.4
CXX = $(BUILD_TOOLS)/bin/g++

INCLUDES = -I$(BUILD_TOOLS)/include
CXXFLAGS = $(INCLUDES) -g  -std=c++14
CXX_LDFLAGS = -Wl,-rpath,$(BUILD_TOOLS)/lib64

LDFLAGS = -L$(BUILD_TOOLS)/lib
BOOST = $(BUILD_TOOLS)

LIBS = $(BOOST)/lib/libboost_system.a \
	$(BOOST)/lib/libboost_filesystem.a \
	$(BOOST)/lib/libboost_timer.a \
	$(BOOST)/lib/libboost_chrono.a \
	$(BOOST)/lib/libboost_iostreams.a \
	$(BOOST)/lib/libboost_regex.a \
	$(BOOST)/lib/libboost_thread.a \
	$(BOOST)/lib/libboost_program_options.a \
	$(BOOST)/lib/libboost_system.a \
	-lpthread

tj: tj.cc
	PATH=$(BUILD_TOOLS)/bin:$$PATH $(CXX)  -g -o $@ $^ $(CXXFLAGS) $(LDFLAGS) $(CXX_LDFLAGS) $(LIBS)

p3x-preload.so: p3x-preload.c
	cc -shared  -fPIC $^ -o $@ -ldl

examp: examp.o
	PATH=$(BUILD_TOOLS)/bin:$$PATH $(CXX) -g -o $@ $^ $(CXXFLAGS) $(LDFLAGS) $(CXX_LDFLAGS) $(LIBS) -lssl -lcrypto


tcli: tcli.o app_client.o app_request.o buffer.o
	PATH=$(BUILD_TOOLS)/bin:$$PATH $(CXX) -g -o $@ $^ $(CXXFLAGS) $(LDFLAGS) $(CXX_LDFLAGS) $(LIBS) -lssl -lcrypto
tcli.o: app_client.h app_request.h buffer.h

p3x-app-shepherd: p3x-app-shepherd.o pidinfo.o app_client.o buffer.o app_request.o
	PATH=$(BUILD_TOOLS)/bin:$$PATH $(CXX) -g -o $@ $^ $(CXXFLAGS) $(LDFLAGS) $(CXX_LDFLAGS) $(LIBS) -lssl -lcrypto

p3x-app-shepherd.o: pidinfo.h clock.h app_client.h buffer.h app_request.h

p3x-calc-start-time-offset: p3x-calc-start-time-offset.c
	$(CC) -g -Wall -o $@ $^

pidinfo.o: pidinfo.h clock.h 

buffer.o: buffer.h

app_client.o: app_client.h app_request.h buffer.h
app_client.h: url_parse.h

app_request.o: app_request.h buffer.h
app_request.h: url_parse.h

pidinfo: pidinfo.cc
	PATH=$(BUILD_TOOLS)/bin:$$PATH $(CXX) -DPIDINFO_TEST_MAIN -g -o $@ $^ $(CXXFLAGS) $(LDFLAGS) $(CXX_LDFLAGS) $(LIBS)

all: 

deploy: deploy-client deploy-service
deploy-all: deploy-client deploy-service
deploy-client: 

deploy-service: 

include $(TOP_DIR)/tools/Makefile.common.rules
