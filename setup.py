from setuptools import find_packages
from skbuild import setup

setup(
    packages=find_packages("py", exclude=["test"]),
    package_dir={"": "py"},
    package_data={"soundswallower": ["py.typed", "_soundswallower.pyi"]},
    entry_points={
        "console_scripts": [
            "soundswallower = soundswallower.cli:main",
        ],
    },
)
