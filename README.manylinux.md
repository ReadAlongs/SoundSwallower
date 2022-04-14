Building binary distributions for Linux
---------------------------------------

To build distributions that are compatible with all the various Linux
distributions, and can therefore be uploaded to PyPI, you can use the
Docker images provided by the [manylinux
project](https://github.com/pypa/manylinux).

The full sequence of commands to create Linux wheels for Python 3.7
through 3.10 is, presuming you use the latest source distribution from
PyPI:

	docker pull quay.io/pypa/manylinux1_x86_64
	docker run -v $PWD:$PWD -w $PWD/dist -it quay.io/pypa/manylinux1_x86_64 /opt/python/cp39-cp39/bin/pip wheel soundswallower
	docker run -v $PWD:$PWD -w $PWD/dist -it quay.io/pypa/manylinux1_x86_64 /opt/python/cp38-cp38/bin/pip wheel soundswallower
	docker run -v $PWD:$PWD -w $PWD/dist -it quay.io/pypa/manylinux1_x86_64 /opt/python/cp37-cp37m/bin/pip wheel soundswallower
	docker pull quay.io/pypa/manylinux2014_x86_64
	docker run -v $PWD:$PWD -w $PWD/dist -it quay.io/pypa/manylinux2014_x86_64 /opt/python/cp310-cp310/bin/pip wheel soundswallower
	for w in dist/*.whl; do docker run -v $PWD:$PWD -w $PWD -it quay.io/pypa/manylinux2014_x86_64 auditwheel repair $w; done

If you wish to use the current directory, replace `soundswallower`
with `.` - likewise, it can also be replaced by the path to an
existing source distribution or source tree.

Note that running auditwheel is necessary to get the platform tags
right, and it will write new wheels in `wheelhouse`.
