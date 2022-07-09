#!/bin/sh

set -e
VERSION=0.3.2
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
python -m build --sdist
docker pull quay.io/pypa/manylinux1_x86_64
for version in cp39-cp39 cp38-cp38 cp37-cp37m; do
    many1_run /opt/python/$version/bin/pip wheel dist/soundswallower-$VERSION.tar.gz
done
docker pull quay.io/pypa/manylinux2014_x86_64
many2014_run /opt/python/cp310-cp310/bin/pip wheel dist/soundswallower-$VERSION.tar.gz
for w in *.whl; do
    many2014_run auditwheel repair -w dist $w
    rm $w
done
