#
# Dockerfile for Crab image with pplite library.
#
# produces package in /crab/build
# Arguments:
#  - UBUNTU:     xenial, bionic (default)
#  - BUILD_TYPE: Debug, Release (default)
#  - BRANCH:     master (default)
#

ARG UBUNTU

# Pull base image.
FROM seahorn/buildpack-deps-crab:$UBUNTU

ARG BRANCH=master
RUN cd / && rm -rf /crab && \
    git clone -b $BRANCH https://github.com/seahorn/crab crab --depth=10 ; \
    mkdir -p /crab/build
WORKDIR /crab/build

ARG BUILD_TYPE
# Build configuration.
RUN cmake -GNinja \
          -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
          -DCMAKE_INSTALL_PREFIX=run \
          -DCMAKE_CXX_COMPILER=g++-6 \
          -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
          -DCRAB_USE_APRON=ON -DCRAB_USE_PPLITE=ON \
	  -DCRAB_ENABLE_TESTS=ON \
          ../ && \
    cmake --build . --target apron  && cmake .. && \
    cmake --build . --target pplite  && cmake .. && \
    cmake --build . --target pplite_domains

# Run tests
RUN /crab/tests/run_tests.sh /crab/tests/expected_results.pplite.out /crab/build

WORKDIR /crab

