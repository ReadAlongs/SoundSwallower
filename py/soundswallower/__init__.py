import os

from ._soundswallower import LogMath
from ._soundswallower import Config
from ._soundswallower import Decoder


def get_model_path():
    """ Return path to the model. """
    return os.path.join(os.path.dirname(__file__), 'model')
