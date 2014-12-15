#!/usr/bin/env python

import os
import time

mxtce_dir = "/usr/local/mxtce-sw"

def shell_execute(cmd, timeout):
  pipe = os.popen(cmd + ' 2>&1', 'r')
  text = ''
  while timeout:
    line = pipe.read()
    text += line
    time.sleep(1)
    timeout = timeout-1
  code = pipe.close()
  if code is None: code = 0
  while (text[-1:] == '\n' or text[-1:] == '\r'):
    text = text[:-1]
  return code, text

file_dir = "%s/resources/topology" % (mxtce_dir)
cmd = "cd %s;svn update" % (file_dir)
print cmd+'\n'
(code, ret) = shell_execute(cmd, 10)
print ret+'\n'
if 'Updated to revision' in ret:
  cmd = "killall -9 mxtce;sleep 3;cd %s;%s/src/main/mxtce -d" % (file_dir, mxtce_dir)
  print cmd+'\n'
  (code, ret) = shell_execute(cmd, 5)
  cmd = "ps -ax|grep mxtce"
  print cmd+'\n'
  (code, ret) = shell_execute(cmd, 1)
  if ' -d' not in ret:
    print "ERROR: failed to restart mxtce!\n"
