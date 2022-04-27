from skbuild import setup
from setuptools import find_packages

with open("README.md", "r", encoding="utf-8") as fh:
    long_description = fh.read()

setup(
    name="soundswallower",
    license="MIT",
    version="0.2",
    description="An even smaller speech recognizer",
    long_description=long_description,
    long_description_content_type="text/markdown",
    author="David Huggins-Daines",
    author_email="dhdaines@gmail.com",
    url="https://github.com/ReadAlongs/SoundSwallower",
    project_urls={
        "Documentation": "https://soundswallower.readthedocs.io/"
    },
    packages=find_packages('py', exclude=["test"]),
    package_dir={"": "py"},
    classifiers=[
        "Development Status :: 2 - Pre-Alpha",
        "Programming Language :: C",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.7",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
    ],
    entry_points={
        "console_scripts": [
            "soundswallower = soundswallower.cli:main",
        ],
    },
)
