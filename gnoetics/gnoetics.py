import os.path, sys

path = os.path.join(os.path.dirname(__file__),
                    "build",
                    "lib.linux-i686-2.2")
sys.path.insert(0, path)
from xxx_gnoetics import *
sys.path.pop(0)

from token import *

from poeticunit import *
from poem  import *

