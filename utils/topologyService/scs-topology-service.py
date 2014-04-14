#!/usr/bin/env python

import os
import sys
import time
import yaml
import omni
import re

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

def get_xml_by_tag(text, tag):
  indx1 = text.find('<'+tag)
  indx2 = text.find('/'+tag+'>')
  xml = None
  if indx1!=-1 and indx2>indx1:
    xml = text[indx1:indx2+len(tag)+2]
  return xml

stream = file('/usr/local/etc/mxtce.config.yaml', 'r')
config = yaml.load(stream)
mxtce_dir = config['mxtceDir']
file_dir = "%s/resources/topology" % (mxtce_dir)
domains = config['tedbManager']['domains']
valid_domains = {}
for k in domains.keys():
  info = domains[k]
  if info['source'] != 'file':
    continue 
  if 'url' not in info:
    continue
  url = info['url']
  xml_file = info['file']
  valid_domains[url] = xml_file

topology_updated = False
for url in valid_domains.keys():
  xml_file = "%s/%s" % (file_dir, valid_domains[url])
  cmd = "omni-get-url-rspec.py %s %s" % (url, xml_file)
  print cmd+'\n'
  (code, ret) = shell_execute(cmd, 30)
  if code != 0:
    print "ERROR: execute command '%s' exits unsuccessfully - error messages below\n %s \n" % (cmd, ret)
    continue
  if '<rspec ' not in ret:
    print "ERROR: failed to retrieve rspec ad from %s \n" % (url)
    continue
  if 'stitching>' not in ret:
    print "ERROR: rspec ad from %s contains no stitching extension \n" % (url)
    continue
  stitching_xml = get_xml_by_tag(ret, "stitching")
  if not stitching_xml:
    stitching_xml = get_xml_by_tag(ret, "ns2:stitching")
  if stitching_xml: 
    stitching_xml = re.sub(r'\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}', '', stitching_xml)
    stitching_xml = re.sub(r'\d{8}:\d{2}:\d{2}:\d{2}', '', stitching_xml)
    base_file = open(xml_file+".base", "w")
    base_file.write(stitching_xml)
    base_file.close()
  else:
    print "ERROR: invalid topology file %s, no %s.base file created \n" % (xml_file, xml_file)
    continue
  cmd = "svn diff %s.base" % (xml_file)
  print cmd+'\n'
  (code, ret) = shell_execute(cmd, 10)
  print ret+'\n'
  if code != 0 and 'not under version control' in ret:
    cmd = "svn add %s %s.base; svn commit -m \"adding %s\" %s %s.base" % (xml_file, xml_file, xml_file, xml_file, xml_file)
    print cmd+'\n'
    (code, ret) = shell_execute(cmd, 10)
    print ret+'\n'
    if code == 0:
      topology_updated = True
  elif code == 0 and '+++' in ret:
    cmd = "svn commit -m \"updating %s\" %s %s.base" % (xml_file, xml_file, xml_file)
    print cmd+'\n'
    (code, ret) = shell_execute(cmd, 10)
    print ret+'\n'
    if code == 0:
        topology_updated = True

if topology_updated:
  cmd = "killall -9 mxtce;sleep 3;cd %s;%s/src/main/mxtce -d" % (file_dir, mxtce_dir)
  print cmd+'\n'
  (code, ret) = shell_execute(cmd, 5)
  cmd = "ps -ax|grep mxtce"
  print cmd+'\n'
  (code, ret) = shell_execute(cmd, 1)
  if ' -d' not in ret:
    print "ERROR: failed to restart mxtce!\n"

