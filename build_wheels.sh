#!/bin/sh

set -e
VERSION=0.1.3

rm -f *.whl dist/*
python setup.py sdist --formats=zip,gztar
docker pull quay.io/pypa/manylinux1_x86_64
docker run -v $PWD:$PWD -w $PWD -it quay.io/pypa/manylinux1_x86_64 /opt/python/cp39-cp39/bin/pip wheel dist/soundswallower-$VERSION.zip
docker run -v $PWD:$PWD -w $PWD -it quay.io/pypa/manylinux1_x86_64 /opt/python/cp38-cp38/bin/pip wheel dist/soundswallower-$VERSION.zip
docker run -v $PWD:$PWD -w $PWD -it quay.io/pypa/manylinux1_x86_64 /opt/python/cp37-cp37m/bin/pip wheel dist/soundswallower-$VERSION.zip
docker pull quay.io/pypa/manylinux2014_x86_64
docker run -v $PWD:$PWD -w $PWD -it quay.io/pypa/manylinux2014_x86_64 /opt/python/cp310-cp310/bin/pip wheel dist/soundswallower-$VERSION.zip
for w in *.whl; do docker run -v $PWD:$PWD -w $PWD -it quay.io/pypa/manylinux2014_x86_64 auditwheel repair $w; done

