from alice.tasklet.convert_version_to_branch_tag.proto import convert_version_to_branch_tag_tasklet


class ConvertVersionToBranchTagImpl(convert_version_to_branch_tag_tasklet.ConvertVersionToBranchTagBase):
    def run(self):
        if self.input.context.version_info.major is not None:
            self.output.state.branch = self.input.context.version_info.major
        else:
            self.output.state.branch = "0"
        if self.input.context.version_info.minor is not None and self.input.context.version_info.minor != '':
            self.output.state.tag = str(int(self.input.context.version_info.minor) + 1)
        else:
            self.output.state.tag = "1"
        self.output.state.success = True
