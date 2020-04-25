import os

from .soundswallower import Config
from .soundswallower import FrontEnd
from .soundswallower import Feature
from .soundswallower import FsgModel
from .soundswallower import JsgfRule
from .soundswallower import LogMath
from .soundswallower import Segment
from .soundswallower import NBest
from .soundswallower import Jsgf
from .soundswallower import Lattice
from .soundswallower import NBestList
from .soundswallower import SegmentList
from .soundswallower import Decoder


DefaultConfig = Decoder.default_config

def get_model_path():
    """ Return path to the model. """
    return os.path.join(os.path.dirname(__file__), 'model')
