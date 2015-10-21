import re
import os
from _psana import DgramList

def getDgramInfo(evt):
    '''returns dict of xtcfilename to offset for datagrams in this event, or None

    Tries to get a psana.DgramList from the event. If successful, returns a 
    dictionary mapping xtcfilenames to offsets for the datagrams comprising the
    event. Otherwise returns None
    '''
    dgList = evt.get(DgramList)
    if dgList is None:
        return None
    xtcfilenames = dgList.getFileNames()
    offsets = dgList.getOffsets()
    return dict([(fname, offset) for fname, offset in zip(xtcfilenames, offsets)])


_regExpressForXtcFileNameStreamChunk = re.compile('^.+-r\d+-s(\d+)-c(\d+)\..*xtc(\..+)*$')

def xtcFileNameStreamChunk(fname):
    '''returns the stream and chunk for a typical xtcfilename, or None, None.

    Filenames must match a regular expression like eNNN-rNNNN-sNN-cNN.*xtc* where N are digits
    If the regex is not matched, None, None is returned.
    '''
    fname = os.path.basename(fname)
    match = _regExpressForXtcFileNameStreamChunk.match(fname)
    if bool(match)==False:
        return None, None
    return int(match.group(1)), int(match.group(2))
