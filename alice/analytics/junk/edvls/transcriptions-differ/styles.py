class Styler:
    def __init__(self, filename=None, code_pattern="\033[%sm", escape_code=None, new_line_symbol="\n",
                 align_symbol=" "):
        # type: (str, str, str, str) ->  Styler
        self.code_pattern = code_pattern
        self._known_styles = dict()
        self.new_line_symbol = new_line_symbol
        self.align_symbol = align_symbol
        if filename:
            self.load_config_file(filename)
        if escape_code is None:
            self.escape_code = self._known_styles["default"]
        else:
            self.escape_code = escape_code

    def load_config_file(self, filename):
        """
        Add styles from file to known styles.
        :param filename: str, path to config file format: "<style_name>\t<code>\n"
        :return: None
        """
        try:
            f = open(filename, 'r')
            for line in f:
                line.split()
                line = line[:-1].partition("\t")
                self.set_style(line[0], line[2])
        except IndexError as error:
            print("Wrong style config file format. File: %s" % filename)
            raise error
        except IOError as error:
            print("Could not open style config file. Filename: %s" % filename)
            raise error

    def style(self, text, style, default_style=None):
        if default_style is not None:
            end_code = self.get_style_code(default_style)
        else:
            end_code = self.escape_code
        style_code = self.get_style_code(style)
        return "".join([style_code, text, end_code])

    def get_style_code(self, style):
        return self._known_styles[style]

    def set_style(self, style, code, **kwargs):
        if "pattern" not in kwargs:
            pattern = self.code_pattern
        else:
            pattern = kwargs["pattern"]
        self._known_styles[style] = pattern % str(code)
