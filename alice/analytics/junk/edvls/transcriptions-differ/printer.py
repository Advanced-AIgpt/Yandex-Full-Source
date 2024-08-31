import sys
import codecs


class Printer:
    def __init__(self, start_message=None, end_message=None, new_line_symbol="\n"):
        self.end_message = end_message
        self.new_line_symbol = new_line_symbol
        if start_message is not None:
            self.print_line(start_message)

    def print_text(self, line):
        raise NotImplementedError()

    def print_line(self, line):
        self.print_text(line)
        self._new_line()

    def _new_line(self):
        self.print_text(self.new_line_symbol)

    def __del__(self):
        if self.end_message is not None:
            self.print_line(self.end_message)


class ConsolePrinter(Printer):
    def __init__(self, start_message=None, end_message=None):
        Printer.__init__(self, start_message, end_message)

    def print_text(self, line):
        sys.stdout.write(line)


class FilePrinter(Printer):
    def __init__(self, file_name, start_message=None, end_message=None, new_line_symbol="\n", encoding="utf8"):
        self._file = codecs.open(file_name, "w", encoding=encoding)
        Printer.__init__(self, start_message, end_message, new_line_symbol)

    def print_text(self, line):
        self._file.write(line)

    def __del__(self):
        if self.end_message is not None:
            self.print_line(self.end_message)
        self._file.close()
