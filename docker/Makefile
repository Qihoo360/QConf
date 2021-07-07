DOCKER=docker
BASE_IMAGE=centos:centos7.2.1511
IMAGE=xidianwlc/qconf:1.2.1

build:
		${DOCKER} pull ${BASE_IMAGE}
		${DOCKER} build -t ${IMAGE} .

push:
		${DOCKER} push ${IMAGE}

test:
		${DOCKER} run --privileged --rm -it ${IMAGE}
