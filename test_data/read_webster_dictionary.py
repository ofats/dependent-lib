import argparse
import json

itemSeparator = '\n@@@\n'
testDataFolder = './test_data'
wordsFile = 'words'


def parseArgs():
    parser = argparse.ArgumentParser(
        description='Parses inputs for benchmark formats\n')
    parser.add_argument('--webster_dictionary',
                        help='Webster dictionary. The dictionary can be downloaded here: https://github.com/adambom/dictionary')
    options = parser.parse_args()
    return options.webster_dictionary


def writeWords(websterDictionary):
    out = open(testDataFolder + '/' + wordsFile, 'w')

    words = list(websterDictionary.keys())
    words.sort()
    for w in words:
        out.write(w)
        out.write(itemSeparator)


if __name__ == '__main__':
    websterDictionaryPath = parseArgs()
    websterDictionary = json.load(open(websterDictionaryPath))

    writeWords(websterDictionary)
