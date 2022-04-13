Full sequence of commands to create Linux wheels and upload them:

	docker pull quay.io/pypa/manylinux1_x86_64
	docker run -v $PWD:$PWD -w $PWD/dist -it quay.io/pypa/manylinux1_x86_64 /opt/python/cp39-cp39/bin/pip wheel soundswallower
	docker run -v $PWD:$PWD -w $PWD/dist -it quay.io/pypa/manylinux1_x86_64 /opt/python/cp38-cp38/bin/pip wheel soundswallower
	docker run -v $PWD:$PWD -w $PWD/dist -it quay.io/pypa/manylinux1_x86_64 /opt/python/cp37-cp37m/bin/pip wheel soundswallower
	docker pull quay.io/pypa/manylinux2014_x86_64
	docker run -v $PWD:$PWD -w $PWD/dist -it quay.io/pypa/manylinux2014_x86_64 /opt/python/cp310-cp310/bin/pip wheel soundswallower
	for w in dist/*.whl; do docker run -v $PWD:$PWD -w $PWD -it quay.io/pypa/manylinux2014_x86_64 auditwheel repair $w; done
	twine upload wheelhouse/*.whl

Note auditwheel is necessary to get the platform tags right, it will write new wheels in `wheelhouse`
