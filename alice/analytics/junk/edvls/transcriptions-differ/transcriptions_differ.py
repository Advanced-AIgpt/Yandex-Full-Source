import codecs
from difflib import SequenceMatcher
from os import listdir
import argparse
import styles
import printer


HTML_START_MESSAGE = """<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>Diff results: %s %s</title>
</head>
<body> <tt>"""
HTML_END_MESSAGE = """</tt></body>
</html>"""
HTML_NEW_LINE = "<br>\n"
HTML_STYLE_CODE_PATTERN = '<span style="color:#%s">'
HTML_STYLE_ESCAPE_CODE = '</span>'
path_to_self = __file__[:(__file__.rfind("/") + 1)]
DEFAULT_CONFIG_FILE = "%sdiffer_styles.cfg" % path_to_self
DEFAULT_HTML_CONFIG_FILE = "%sdiffer_html_styles.cfg" % path_to_self
PAUSES_TO_BE_IGNORED = ["END OF UTT (, )", "- (NOPOS, default) pau", "- (NOPOS, default)  pau", "- (, ) pau"]
HTML_ALIGN_SYMBOL = '&nbsp;'
IGNORE_POS_DICT = {(1, "start"): "(", (1, "end"): ",", (2, "end"): ")"}


class ErrorSentencesAreDifferent(Exception):
    pass


def corresponding_words_map(first_list, second_list, is_junk=None):
    """Get 2 lists of strings l1, l2. Return list r: l1[i]~~l2[r[i]], r[i] raises.
    If no matching word could be defined in l2 l1[i] = next to last matching position"""
    diff_words = SequenceMatcher(is_junk, second_list, first_list, autojunk=False)
    matching_words = diff_words.get_matching_blocks()
    matching_words_cursor = 0
    in_block_cursor = 0
    correspond_words = []
    for i in range(len(first_list)):
        if i >= matching_words[matching_words_cursor][1] + matching_words[matching_words_cursor][2]:
            matching_words_cursor += 1
            in_block_cursor = 0
        if i < matching_words[matching_words_cursor][1]:
            if matching_words_cursor == 0:
                correspond_words.append(0)
            elif matching_words[matching_words_cursor][1]:
                correspond_words.append(min(len(second_list) - 1,
                                            matching_words[matching_words_cursor - 1][0] +
                                            matching_words[matching_words_cursor - 1][2]))
        else:
            correspond_words.append(matching_words[matching_words_cursor][0] + in_block_cursor)
            in_block_cursor += 1
    return correspond_words


class Differ:
    def __init__(self, styler=None, filler_char="*", lines_delimiter="---", ignore_pauses_flag=False, ignore_pos_flag=False):
        if styler is None:
            styler = styles.Styler()
        self.styler = styler
        self.filler_char = filler_char
        self.lines_delimiter = lines_delimiter + self.styler.new_line_symbol
        self.ignore_pauses_flag = ignore_pauses_flag
        self.ignore_pos_flag = ignore_pos_flag

    def diff_lines(self, old_line, new_line):
        """ return string difference representation """
        old_line = old_line.split()
        new_line = new_line.split()
        opcodes = SequenceMatcher(lambda x: x.isspace(), old_line, new_line, autojunk=False).get_opcodes()
        diff = []
        for i in range(len(opcodes)):  # diff contains opcodes with equal lengths of "replace" intervals
            tag, i1, i2, j1, j2 = opcodes[i]
            if tag == "replace" and i2 - i1 != j2 - j1:
                if i2 - i1 < j2 - j1:
                    diff.append(("replace", i1, i2, j1, j1 + (i2 - i1)))
                    diff.append(("insert", i2, i2, j1 + (i2 - i1), j2))
                else:
                    diff.append(("replace", i1, i1 + (j2 - j1), j1, j2))
                    diff.append(("delete", i1 + (j2 - j1), i2, j2, j2))
            else:
                diff.append(opcodes[i])

        result = [[], []]
        for tag, i1, i2, j1, j2 in diff:
            if tag == "equal":
                result[0] += old_line[i1:i2]
                result[1] += new_line[j1:j2]
            elif tag == "replace":
                for i in range(i2 - i1):
                    old_word = old_line[i1 + i]
                    new_word = new_line[j1 + i]
                    len_diff = len(new_word) - len(old_word)  # length aligning
                    if len_diff >= 0:
                        old_word += self.styler.align_symbol * len_diff
                    else:
                        new_word += self.styler.align_symbol * (-len_diff)
                    result[0].append(self.styler.style(old_word, "substitution_old"))
                    result[1].append(self.styler.style(new_word, "substitution_new"))
            elif tag == "delete":
                for i in range(i1, i2):
                    old_word = old_line[i]
                    result[0].append(self.styler.style(old_word, "deletion_old"))
                    result[1].append(self.styler.style(self.filler_char + self.styler.align_symbol * (len(old_word) - 1), "deletion_new"))
            elif tag == "insert":
                for j in range(j1, j2):
                    new_word = new_line[j]
                    result[0].append(self.styler.style(self.filler_char + self.styler.align_symbol * (len(new_word) - 1), "insertion_old"))
                    result[1].append(self.styler.style(new_word, "insertion_new"))

        for i in range(len(result)):
            # result[i] = u"".join(map(lambda x: x + self.styler.align_symbol, result[i]))[:-1]
            result[i] = self.styler.align_symbol.join(result[i])
        return "".join([result[0], self.styler.new_line_symbol, result[1], self.styler.new_line_symbol])

    def diff_files(self, old_file, new_file):
        sentence = old_file.readline().strip()
        new_file.readline()
        # if sentence != new_file.readline().strip():
        #    raise ErrorSentencesAreDifferent
        old_lines = filter(None, map(lambda x: x.strip(), list(old_file)))
        new_lines = filter(None, map(lambda x: x.strip(), list(new_file)))
        if self.ignore_pauses_flag:
            old_lines = filter(lambda x: x not in PAUSES_TO_BE_IGNORED, old_lines)
            new_lines = filter(lambda x: x not in PAUSES_TO_BE_IGNORED, new_lines)
        if self.ignore_pos_flag:
            for lines in old_lines, new_lines:
                for i in range(len(lines)):
                    line = lines[i]
                    line_list = line.split()
                    to_be_edited = set()
                    for key, pattern in IGNORE_POS_DICT.iteritems():
                        pos, option = key
                        to_be_edited.add(pos)
                        word = line_list[pos]
                        if option == "start":
                            if not word.startswith(pattern):
                                to_be_edited = False
                                break
                        elif option == "end":
                            if not word.endswith(pattern):
                                to_be_edited = False
                                break
                        else:
                            exit("Invalid IGNORE_POS_DICT constant.")
                    if to_be_edited:
                        to_be_edited = list(to_be_edited)
                        for pos in reversed(to_be_edited):
                            line_list.pop(pos)
                        line = " ".join(line_list)
                    lines[i] = line
        words_from_sentence = []
        word = []
        for c in sentence:
            if c.isalpha() or c.isdigit():
                word.append(c)
            else:
                if c == ' ':
                    word.append(' ')
                if len(word) != 0 and word[0] != ' ':
                    words_from_sentence.append("".join(word))
                if c == ' ':
                    word = []
                else:
                    word = [c]
        if len(word) != 0 and word[0] != ' ':
            words_from_sentence.append("".join(word))
        correspond_words_old = corresponding_words_map(map(lambda x: x.split()[0], old_lines),
                                                       map(lambda x: x.lower().strip(), words_from_sentence),
                                                       lambda x: x.isspace())
        correspond_words_new = corresponding_words_map(map(lambda x: x.split()[0], new_lines),
                                                       map(lambda x: x.lower().strip(), words_from_sentence),
                                                       lambda x: x.isspace())
        opcodes = SequenceMatcher(lambda x: x.isspace(), map(lambda x: x.split()[0], old_lines),
                                  map(lambda x: x.split()[0], new_lines), autojunk=False).get_opcodes()
        # map(lambda x: "".join(filter(lambda y: not y.isspace(), x.split())), old_lines),
        diff = []
        for i in range(len(opcodes)):  # diff contains opcodes with equal lengths of "replace" intervals
            tag, i1, i2, j1, j2 = opcodes[i]
            if tag == "replace" and i2 - i1 != j2 - j1:
                if i2 - i1 < j2 - j1:
                    diff.append(("replace", i1, i2, j1, j1 + (i2 - i1)))
                    diff.append(("insert", i2, i2, j1 + (i2 - i1), j2))
                else:
                    diff.append(("replace", i1, i1 + (j2 - j1), j1, j2))
                    diff.append(("delete", i1 + (j2 - j1), i2, j2, j2))
            elif tag == "equal":
                for cursor in range(i2 - i1):
                    if ("".join(filter(lambda y: not y.isspace(), old_lines[i1 + cursor].split())) !=
                            "".join(filter(lambda y: not y.isspace(), new_lines[j1 + cursor].split()))):
                        new_tag = "replace"
                    else:
                        new_tag = "equal"
                    diff.append((new_tag, i1 + cursor, i1 + cursor + 1, j1 + cursor, j1 + cursor + 1))
            else:
                diff.append(opcodes[i])
        if all(map(lambda x: x[0] == "equal", diff)):
            return
        result = [""]
        result_sentence_styles = ['equal_sentence'] * len(words_from_sentence)  # style of sentence words
        for tag, i1, i2, j1, j2 in diff:
            if tag == "equal":
                continue
            elif tag == "replace":
                for pair in zip(old_lines[i1:i2], new_lines[j1:j2]):
                    result.append(self.diff_lines(*pair))
                    result.append(self.lines_delimiter)
                for i in range(i1, i2):
                    if correspond_words_old[i] is not None:
                        result_sentence_styles[correspond_words_old[i]] = "substitution_sentence"

            elif tag == "insert":
                for j in range(j1, j2):
                    line = new_lines[j]
                    first_word = line.split()[0]
                    result += [self.styler.style(self.filler_char, "insertion_old"), self.styler.new_line_symbol]
                    result.append(self.styler.style(first_word, "insertion_new"))
                    result.append(line[len(first_word):])
                    result += [self.styler.new_line_symbol, self.lines_delimiter]

                    #  if correspond_words_new[j] is not None:
                    result_sentence_styles[correspond_words_new[j]] = "insertion_sentence"
            else:  # tag == "delete"
                for i in range(i1, i2):
                    line = old_lines[i]
                    first_word = line.split()[0]
                    result.append(self.styler.style(first_word, "deletion_old"))
                    result.append(line[len(first_word):])
                    result += [self.styler.new_line_symbol, self.styler.style(self.filler_char, "deletion_new"),
                               self.styler.new_line_symbol, self.lines_delimiter]
                    #  if correspond_words_old[i] is not None:
                    result_sentence_styles[correspond_words_old[i]] = "deletion_sentence"
        result[0] = result[0].join(map(self.styler.style, words_from_sentence, result_sentence_styles))
        result[0] += self.styler.new_line_symbol
        return "".join(result)


def main():
    parser = argparse.ArgumentParser(description="Differ for transcriptions generated with batch.py.")
    parser.add_argument("path_old_ver", type=str, help="Path to old transcriptions.")
    parser.add_argument("path_new_ver", type=str, help="Path to new transcriptions.")
    parser.add_argument("--postfix", type=str, default=".txt",
                        help="Name ending of files. Default is '.txt'")
    parser.add_argument("--ignore_not_paired", default=False, action="store_const", const="True",
                        help="Go on if some files have no pairs. Warnings will be shown.")
    parser.add_argument("--style_config", type=str, default="",
                        help="Path to style config file. Default is 'differ_styles.cfg' or 'differ_html_styles.cfg' " +
                             "if --html flag is set.")
    parser.add_argument("--output_file", "-O", default="", type=str, help="Name for output file.")
    parser.add_argument("--html", default=False, action="store_const", const=True,
                        help="Use html output.")
    parser.add_argument("--ignore_pauses", default=False, action="store_const", const=True,
                        help="Ignore 'END OF UTT' and 'pau' insertions/deletions")
    parser.add_argument("--ignore_pos", default=False, action="store_const", const=True,
                        help="Ignore part of speech differences. Use this if you comparing test and prod.")

    args = parser.parse_args()
    postfix = args.postfix
    html_flag = args.html
    old_path = args.path_old_ver
    ignore_pauses_flag = args.ignore_pauses
    ignore_pos_flag = args.ignore_pos
    if args.style_config is "":
        if html_flag:
            config_file = DEFAULT_HTML_CONFIG_FILE
        else:
            config_file = DEFAULT_CONFIG_FILE
    else:
        config_file = args.style_config
    if old_path[-1] != "/":
        old_path += "/"
    new_path = args.path_new_ver
    if new_path[-1] != "/":
        new_path += "/"
    if args.output_file:
        output_file = args.output_file
        print("Output will be written to %s" % output_file)
        if html_flag:
            output = printer.FilePrinter(output_file, start_message=(HTML_START_MESSAGE % (old_path, new_path)),
                                         end_message=HTML_END_MESSAGE, new_line_symbol=HTML_NEW_LINE)
        else:
            output = printer.FilePrinter(output_file)
    else:
        output = printer.ConsolePrinter()
    files, new_files = None, None
    try:
        files = set(filter(lambda x: x[-len(postfix):] == postfix, listdir(old_path)))
    except OSError:
        exit("Could not open old path.")
    try:
        new_files = set(filter(lambda x: x[-len(postfix):] == postfix, listdir(new_path)))
    except OSError:
        exit("Could not open new path.")

    if files != new_files:
        print("WARNING: There are files with no pair:")
        for i in files - new_files:
            print (old_path + i)
        for i in new_files - files:
            print (new_path + i)
        if not args.ignore_not_paired:
            exit("Files with no pair. Use --ignore_not_paired option if it is OK.")
        else:
            print("Files with no pair will be ignored.")
            print("")
            files = files.intersection(new_files)
    files = list(files)
    files.sort()
    if html_flag:
        styler = styles.Styler(config_file, code_pattern=HTML_STYLE_CODE_PATTERN, escape_code=HTML_STYLE_ESCAPE_CODE,
                               new_line_symbol=HTML_NEW_LINE, align_symbol=HTML_ALIGN_SYMBOL)
    else:
        styler = styles.Styler(config_file)
    differ = Differ(styler, ignore_pauses_flag=ignore_pauses_flag, ignore_pos_flag=ignore_pos_flag)
    difference = ""
    for file_name in files:
        old_file = codecs.open(old_path + file_name, 'r', encoding='utf-8')
        new_file = codecs.open(new_path + file_name, 'r', encoding='utf-8')
        try:
            difference = differ.diff_files(old_file, new_file)
        except ErrorSentencesAreDifferent:
            exit("Sentences (first line) in files %s are different." % file_name)
        if difference:
            output.print_line(file_name)
            output.print_line(difference)

if __name__ == "__main__":
    main()
