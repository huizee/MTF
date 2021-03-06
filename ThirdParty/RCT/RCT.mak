RCT_ROOT_DIR = ThirdParty/RCT
RCT_SRC_DIR = ${RCT_ROOT_DIR}/src
RCT_INCLUDE_DIR = ${RCT_ROOT_DIR}/include
RCT_HEADER_DIR = ${RCT_INCLUDE_DIR}/mtf/${RCT_ROOT_DIR}

RCT_HEADERS = $(addprefix  ${RCT_HEADER_DIR}/, CompressiveTracker.h)

THIRD_PARTY_TRACKERS += CompressiveTracker
THIRD_PARTY_HEADERS += ${RCT_HEADERS}
THIRD_PARTY_INCLUDE_DIRSTHIRD_PARTY_INCLUDE_DIRS += ${RCT_INCLUDE_DIR}
THIRD_PARTY_INCLUDE_DIRS += ${RCT_INCLUDE_DIR}

${BUILD_DIR}/CompressiveTracker.o: ${RCT_SRC_DIR}/CompressiveTracker.cc ${RCT_HEADERS} ${UTILITIES_HEADER_DIR}/miscUtils.h ${MACROS_HEADER_DIR}/common.h ${ROOT_HEADER_DIR}/TrackerBase.h
	${CXX} -c -fPIC ${WARNING_FLAGS} ${OPT_FLAGS} ${MTF_COMPILETIME_FLAGS} $< ${FLAGS64} ${FLAGSCV} -I${RCT_INCLUDE_DIR} -I${UTILITIES_INCLUDE_DIR} -I${MACROS_INCLUDE_DIR} -I${ROOT_INCLUDE_DIR} -o $@
	