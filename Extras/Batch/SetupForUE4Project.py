# Copyright 2017 - 2018 Code 4 Game, Org. All Rights Reserved.

'''setup the environment for ue4 project'''
import sys
import os
import argparse
import ConfigParser
import shutil

def doCopy(configParser, ue4Version, folderPath):
    '''do copy by config'''
    if configParser.has_section(ue4Version):
        if configParser.has_option(ue4Version, u"from"):
            doCopy(configParser, configParser.get(ue4Version, u"from"), folderPath)
        for key, value in configParser.items(ue4Version):
            if key == u"from":
                continue
            src_path = os.path.abspath(os.path.join(folderPath, key))
            dst_path = os.path.abspath(os.path.join(folderPath, value))
            try:
                shutil.copyfile(src_path, dst_path)
            except Exception as ex:
                print u"failed to copy file from", src_path, u"to", dst_path, u"by", ex.message
                sys.exit(1)

def main(argv):
    '''main function'''
    arg_parser = argparse.ArgumentParser()
    arg_parser.add_argument(u"configFile", type=str)
    arg_parser.add_argument(u"ue4Version", type=str)
    args = arg_parser.parse_args(argv)

    with open(args.configFile, u'r') as config_file:
        config_parser = ConfigParser.RawConfigParser(allow_no_value=True)
        config_parser.optionxform = str
        config_parser.readfp(config_file)

        folder_path = os.path.dirname(os.path.abspath(args.configFile))
        doCopy(config_parser, args.ue4Version, folder_path)

if __name__ == u"__main__":
    main(sys.argv[1:])
