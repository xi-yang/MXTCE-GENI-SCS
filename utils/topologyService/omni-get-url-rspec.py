#!/usr/bin/env python

# usage: ./omni-get-url-rspec.py  http://max-myplc.dragon.maxgigapop.net:12346 ./max1.xml 2>&1 |tee > /dev/nul

import sys
import omni

def main(argv=None):
  url = sys.argv[1]
  file = sys.argv[2]
  args = ['listresources', '-a', url]
  (options, args) = omni.parse_args(args)
  (text, ret) = omni.call(args, options)
  if url in ret and 'value' in ret[url]:
    rspec = ret[url]['value']
    rspec_file = open(file, "w")
    rspec_file.write(rspec)
    rspec_file.close()
    return 0
  return 1

if __name__ == "__main__":
  sys.exit(main())

