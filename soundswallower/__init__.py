import os

from .soundswallower import *

DefaultConfig = Decoder.default_config


def get_model_path():
    """ Return path to the model. """
    return os.path.join(os.path.dirname(__file__), 'model')
