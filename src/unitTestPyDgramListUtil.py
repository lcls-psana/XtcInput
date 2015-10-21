import unittest
from XtcInput.PyDgramListUtil import xtcFileNameStreamChunk

class XtcFileNameStreamChunk( unittest.TestCase ):
    def setUp(self):
        pass

    def tearDown(self):
        pass

def test_regexp(self):
    '''test the regular expression for matching
    '''
    checks = [('e642-r0128-s80-c00.smd.xtc.inprogress', '80', '00'),
              ('dir/e642-r0128-s80-c00.smd.xtc.inprogress', '80', '00'),
              ('/a/b/c/d/dir/e642-r0128-s80-c00.smd.xtc.inprogress', '80', '00'),
              ('dir/e6asdfa42-r18-s02-c4.xtc', '02', '4'),
              ('dir/e6asdfa42-r18-s02-c4.xtc.inprogress', '02', '4'),
              ('e6asdfa42-r18-s02-c4.xtcx', None, None),
              ('e6asdfa42-r18_s02-c4.xtc.inprogress', None, None),
              ('e6asdfa42-r18_s02-c4g.xtc.inprogress', None, None),
              ('e6asdfa42-r18_s02-c4', None, None)]
    for fname, stream, chunk in checks:
        streamFromFunc, chunkFromFunc = xtcFileNameStreamChunk(fname)
        if stream is None:
            self.assertIsNone(streamFromFunc, msg="should not have gotten stream for fname=%s" % fname)
        else:
            self.assertEqual(streamFromFunc, stream, msg="stream from func != expected for fname=%s" % fname)
        if chunk is None:
            self.assertIsNone(chunkFromFunc, msg="should not have gotten chunk for fname=%s" % fname)
        else:
            self.assertEqual(chunkFromFunc, chunk, msg="chunk from func != expected for fname=%s" % fname)

if __name__ == "__main__":

    # now run the tests, this routine does not return.
    unittest.main(argv=[sys.argv[0], '-v'])

