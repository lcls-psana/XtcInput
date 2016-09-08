import sys
import os
import io
import re
import math

from XtcInput.PyDgramListUtil import xtcFileNameStreamChunk, getDgramInfo

class LiveAvail(object):
    '''psana module to find out how far behind psana is from the live events 
    being written to disk. 

    To use LiveAvail, an instance of the class must be passed to the DataSource
    module list. Example:

    Usage::

        import psana
        liveAvail = psana.LiveAvail()

        ds = psana.DataSource("exp=xpptut13:run=39:smd:live", module=liveAvail)
        for evt in ds.events():
            if liveAvail.toFarBehind(): continue
            # process event
    
    The method toFarBehind() will return True when reading live data, and the most 
    recent event obtained appears to be far behind the most recent events available
    on disk.

    If one is not in live mode, or in live mode but reading files that have been 
    completely written to disk, toFarBehind() always returns False.

    The steps are:

       * constructs an instance of LiveAvail with all default arguments
       * add the liveAvail instance to the DataSource module list, via the 'module' parameter
       * call the toFarBehind() method to decide if you want to 'skip' an event
         to catch up to the most recent events on disk

    Notes:
      * the lag can be fine tuned (see Init docstring)
      * availabe events are estimated using average event size from ONE DAQ stream
      * The one stream approach allows for an efficient implementation, however the results
        are less reliable when the DAQ is handling large amounts of damage.

    '''
    def __init__(self, eventLag='medium',  # can be 'short' or 'long'
                 eventLagInEvents=None,    # in number of events
                 trace=False, 
                 debug=False, 
                 eventsForStats = 60):
        '''allows one to skip events to keep up with live data
        
        The default is to return True only when one gets within 250 events of the end 
        (which is about 2 seconds at 120hz). It is reccommended to use the eventLag
        parameter - 'medium', 'short' or 'long'. Although one can specify eventLagInEvents
        directly, it is easy to skip too many events. One should not expect 120 writes
        per second. If there were only 2 writes a second, one would get 60 new events at
        a time. keeping up with the most recent event would mean skipping 59 out of 60 events.

        ARGS:
          
          eventLag (str): one of 'medium' 250 events ~ 2 seconds, default
                                 'short'  130 events ~ 1 second
                                 'long'   500 events ~ 4 seconds
          eventLagInEvents (int): one can override the eventLag parameter by providing
                                  ones own number of events, however this can be tricky
                                  to get correct

          trace, debug (bool): set to True to get additional, debugging output

          eventsForStats (int): toFarBehind() always returns False during the first
               few events while it builds up a average size for an event. The default
               value should always be fine for small data, and most cases when reading
               large data. However when there is a slow large detector (3 hz or slower)
               one should increase this to say 130.
        '''

        # _psanaInternalStreamBufferLen is the length of the internal buffer
        # that psana maintains for each stream. Psana does not deliver any events
        # to the user until each of these stream buffer contains at least this many
        # events. 
        self._psanaInternalStreamBufferLen = 20

        assert eventLag in ['medium', 'short','long'], \
            "eventLag parameter must be one of 'short', 'medium', or 'long'"
        if eventLagInEvents is not None:
            assert isinstance(eventLagInEvents, int), "eventLagInEvents parameter must be an integer"
            assert eventLagInEvents > 0, "eventLagInEvents must be positive"
            self._eventLag = eventLagInEvents
        else:
            if eventLag == 'medium':
                self._eventLag = 250
            elif eventLag == 'short':
                self._eventLag = 130
            elif eventLag == 'long':
                self.__eventLag = 500
                
        self._eventsForStats = eventsForStats
        self._debug = debug
        self._trace = trace or debug
        self._eventNumber = -1
        self._beginRunCalled = False
        self._doLiveAvail = False
        self._examinedDaqStream = None
        self._examinedDaqStreamInfo = {'fname':None,
                                       'chunk':None,
                                       'fileobj':None,
                                       'last_off':None,
                                       'dgramsize':None,
                                       'num_complete_dgrams':None}
        self._totalNumberOfDaqStreams = None
        self._newDgramWeight = 0.01
        self.tracemsg("eventLag=%d" % self._eventLag)

    def __del__(self):
        if self._examinedDaqStreamInfo is None:
            return
        fh = self._examinedDaqStreamInfo['fileobj']
        if fh is None: 
            return
        if not fh.closed:
            self.tracemsg("__del__: closing open filehandle for %s" % \
                         self._examinedDaqStreamInfo['fname'])
        fh.close()

    def tracemsg(self, msg):
        if self._trace:
            sys.stdout.write("trace: LiveAvail: %s\n" % msg)

    def debugmsg(self, msg):
        if self._debug:
            sys.stdout.write("debug: LiveAvail: %s\n" % msg)

    def errormsg(self, msg):
        sys.stderr.write("ERROR: LiveAvail: %s\n" % msg)

    def warnmsg(self, msg):
        sys.stderr.write("WARN: LiveAvail: %s\n" % msg)

    def _identifyInProgressDaqStreams(self, file2offset):
        '''helper to beginrun.

        ARGS:
          file2offset: result of parsing psana.DgramList from beginrun

        RET: tuple of two items. If there is an error parsing, 
             returns None, None, otherwise the two objects:

          inProgressDaqStreams    - dict, keys: streams < 80 
                                          values: xtcfilenames ending with .inprogress
          notInProgressDaqStreams - set,  streams < 80 where filename did not end with
                                          .inprogress
        '''
        inProgressDaqStreams = {}
        notInProgressDaqStreams = set()

        for fname, offset in file2offset.iteritems():
            stream, chunk = xtcFileNameStreamChunk(fname)
            if stream is None or chunk is None:
                self.errormsg("could not parse the xtcfilename=%s from Dglist" % fname)
                return None, None
            if chunk != 0:
                self.warnmsg(("In beginrun, but chunk for xtcfilename=%s is not 0."
                              "availEvents() will always return 0") % fname);
                return None, None
            if stream >= 80:
                continue
            if fname.endswith('.inprogress'):
                inProgressDaqStreams[stream] = fname
            else:
                notInProgressDaqStreams[stream] = fname

        return inProgressDaqStreams, notInProgressDaqStreams

    def beginrun(self, evt, env):
        self._beginRunCalled = True
        file2offset = getDgramInfo(evt)
        if file2offset is None or len(file2offset)==0:
            self.errormsg("No psana.DgramList, or empty list found in event during beginrun")
            return

        inProgressDaqStreams, notInProgressDaqStreams = self._identifyInProgressDaqStreams(file2offset)
        if inProgressDaqStreams is None: 
            return
        
        if len(inProgressDaqStreams)>0 and len(notInProgressDaqStreams)>0:
            self.errormsg("beginrun: there is a mix of .inprogress files, "
                          "and not in progress files. Very unexpected. "
                          "Will not process. liveAvail will always return False")
            return

        if len(inProgressDaqStreams)==0:
            self.warnmsg("beginrun: there are no .inprogress files. "
                         "Chunk 0 files must all be on disk. "
                         "Will assume NOT live mode. liveAvail will always return False.")
            return
            
        self._examinedDaqStream = min(inProgressDaqStreams.keys())
        fname = inProgressDaqStreams[self._examinedDaqStream]
        stream, chunk = xtcFileNameStreamChunk(fname)
        assert stream == self._examinedDaqStream
        try:
            fh = io.open(fname,'rb')
        except IOError,e:
            self.errormsg("beginrun: could not open file " + \
                          ("%s.\nIOError: %s\nLiveAvail always returning False" % \
                          (fname, e)))
            return

        streamInfo = {'fname':fname,
                      'chunk':chunk,
                      'fileobj':fh,
                      'last_off':0,
                      'dgramsize':None,
                      'num_complete_dgrams':0
                      }

        if (self._examinedDaqStreamInfo is not None) and \
           (self._examinedDaqStreamInfo['fileobj'] is not None):
           self._examinedDaqStreamInfo['fileobj'].close()
        self._examinedDaqStreamInfo = streamInfo

        self._totalNumberOfDaqStreams = len(inProgressDaqStreams) + \
                                        len(notInProgressDaqStreams)
        self._doLiveAvail = True

    def event(self, evt, env):
        self._eventNumber += 1

        if not self._doLiveAvail:
            return

        fname2offset = getDgramInfo(evt)
        if fname2offset is None:
            self.errormsg("dglist is None in event %d" % self.eventNumber)
            return 

        for fname, offset in fname2offset.iteritems():
            stream, chunk = xtcFileNameStreamChunk(fname)

            if stream is None or chunk is None:
                self.errormsg("event: cannot parse fname: %s" % fname)
                return False

            if stream != self._examinedDaqStream:
                continue

            if offset < 0:
                self.errormsg("event: offset=%s is < 0. not processing for stream=%d fname=%s" % \
                              (offset, stream, fname))
                continue

            if chunk == self._examinedDaqStreamInfo['chunk']:
                self._updateStreamInfoSameChunk(self._examinedDaqStreamInfo, offset)
            else:
                self._updateStreamInfoNewChunk(self._examinedDaqStreamInfo, fname, chunk, offset)

    def _updateStreamInfoNewChunk(self, streamInfo, fname, chunk, offset):
        streamInfo['chunk'] = chunk
        streamInfo['last_off'] = offset

        if streamInfo['fileobj'] is not None:
            streamInfo['fileobj'].close()
            streamInfo['fileobj'] = None

        try:
            streamInfo['fileobj'] = io.open(fname, 'rb')
        except IOError,ioError:
            if fname.endswith('.inprogress'):
                self.warnmsg("event:. chunk changed. Failed to open a "
                             "inprogress file associated with event. Trying "
                             "filename without .inprogress extension. This is "
                             "normal if falling far behind, or next chunk is "
                             "very short.")
                fname == fname[0:-11]
                streamInfo['fname'] = fname
                try:
                    streamInfo['fileobj'] = io.open(fname, 'rb')
                except IOError, ioError:
                    self.errormsg("evente: chunk changed, but could "
                                  "open neither inprogress nor not inprogress. "
                                  "liveAvail() will always return False");
                    self._doLiveAvail = False
        self.tracemsg("changed chunk. fname=%s chunk=%d offset=%d" % \
                      (fname, chunk, offset))
            
                           
    def _updateStreamInfoSameChunk(self, streamInfo, offset):
        if streamInfo['last_off'] == 0:
            streamInfo['last_off'] = offset
            return

        if offset > streamInfo['last_off']:
            lastDgramSize = offset - streamInfo['last_off']
            streamInfo['last_off'] = offset
            if streamInfo['num_complete_dgrams'] == 0:
                streamInfo['num_complete_dgrams'] = 1
                streamInfo['dgramsize'] = lastDgramSize
            else:
                streamInfo['dgramsize'] *= self._newDgramWeight
                streamInfo['dgramsize'] += (1.0-self._newDgramWeight) * lastDgramSize
                streamInfo['num_complete_dgrams'] += 1


    def toFarBehind(self):
        '''returns true if lag in events is too far behind.
        '''
        if not self._beginRunCalled:
            self.errormsg("beginrun not called. Is module in DataSource module= list?")
            return False
        
        if not self._doLiveAvail:
            return False

        if self._eventNumber < self._eventsForStats:
            return False

        if self._examinedDaqStreamInfo['fileobj'] is None:
            return False

        if self._examinedDaqStreamInfo['dgramsize'] is None:
            return False

        assert self._examinedDaqStreamInfo['num_complete_dgrams'] > 0
        numDgramsToGetPastPsanaBuffer = 1  # for the current event, offset was the start of it
        numDgramsToGetPastPsanaBuffer += self._psanaInternalStreamBufferLen
        
        numInEachDaqStream = int(math.ceil(self._eventLag/float(self._totalNumberOfDaqStreams)))
        
        numDgramsForTrue = numInEachDaqStream + numDgramsToGetPastPsanaBuffer
        bytesFromLastOffsetForTrue = int(math.ceil(numDgramsForTrue * self._examinedDaqStreamInfo['dgramsize']))

        fileLengthForTrue = self._examinedDaqStreamInfo['last_off'] + bytesFromLastOffsetForTrue

        fileLength = self._examinedDaqStreamInfo['fileobj'].seek(0, io.SEEK_END)

        result = fileLength > fileLengthForTrue

        if self._debug:
            bytesOnDisk = fileLength - self._examinedDaqStreamInfo['last_off']
            dgramsOnDisk = bytesOnDisk/self._examinedDaqStreamInfo['dgramsize']
            self.debugmsg("event=%d toFarBehind()=%d (based on eventLag=%d or dgrams_this_stream=%d)dgramsOnDisk=%.1f filelength=%.2f mb" % \
                          (self._eventNumber, result, self._eventLag, numDgramsForTrue, dgramsOnDisk, fileLength/float(1<<20)))

        return result
            
        

