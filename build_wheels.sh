#!/bin/sh

set -e
VERSION=0.2
U=$(id -u)
G=$(id -g)

many1_run() {
    docker run -v $PWD:$PWD -v $HOME/.cache/pip:/.cache/pip -u $U:$G -w $PWD -it quay.io/pypa/manylinux1_x86_64 "$@"
}
many2014_run() {
    docker run -v $PWD:$PWD -v $HOME/.cache/pip:/.cache/pip -u $U:$G -w $PWD -it quay.io/pypa/manylinux2014_x86_64 "$@"
}
	       

python setup.py clean
rm -rf *.whl dist/* _skbuild py/soundswallower.egg-info
python setup.py sdist --formats=zip
docker pull quay.io/pypa/manylinux1_x86_64
many1_run /opt/python/cp39-cp39/bin/pip wheel dist/soundswallower-$VERSION.zip
many1_run /opt/python/cp38-cp38/bin/pip wheel dist/soundswallower-$VERSION.zip
many1_run /opt/python/cp37-cp37m/bin/pip wheel dist/soundswallower-$VERSION.zip
docker pull quay.io/pypa/manylinux2014_x86_64
many2014_run /opt/python/cp310-cp310/bin/pip wheel dist/soundswallower-$VERSION.zip
for w in *.whl; do
    docker run -v $PWD:$PWD -u $U:$G -w $PWD -it \
	   quay.io/pypa/manylinux2014_x86_64 auditwheel repair $w
done

