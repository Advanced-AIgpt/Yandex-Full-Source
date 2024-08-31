#include <library/cpp/getopt/last_getopt.h>
#include <kernel/keyinv/indexfile/indexfile.h>
#include <kernel/keyinv/indexfile/indexwriter.h>
#include <library/cpp/wordpos/wordpos.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <kernel/tarc/docdescr/docdescr.h>
#include <kernel/tarc/iface/farcface.h>

#include <util/datetime/base.h>
#include <util/folder/path.h>
#include <util/stream/file.h>

int main(int argc, char **argv) {
    TFsPath indexDir;
    NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();
    opts
        .AddLongOption('d', "index-dir")
        .RequiredArgument("DIR")
        .Required()
        .StoreResult(&indexDir);

    opts.AddHelpOption('h');
    opts.SetFreeArgsNum(0);
    NLastGetopt::TOptsParseResult parsedOpts(&opts, argc, argv);

    indexDir.MkDirs();

    // write dummy indexinv/indexkey files
    NIndexerCore::TOutputIndexFile ofile(IYndexStorage::FINAL_FORMAT);
    ofile.Open((indexDir / "index").c_str());
    NIndexerCore::TInvKeyWriter writer(ofile);
    ofile.CloseEx();

    TString arcFile = (indexDir / "indexarc").c_str();
    TString dirFile = (indexDir / "indexdir").c_str();
    TFixedBufferFileOutput output(arcFile);
    WriteTextArchiveHeader(output);

    TString line;
    size_t docId = 0;
    size_t numTurns = 0;
    while (Cin.ReadLine(line)) {
        size_t curNumTurns = std::count(line.data(), line.data() + line.size(), '\t');
        if (numTurns == 0) {
            numTurns = curNumTurns;
        }
        Y_VERIFY(numTurns == curNumTurns);

        TStringBuf source;
        TStringBuf gifSourceUrl;
        TStringBuf gifSourceText;
        TStringBuf gifUrl;
        TStringBuf proactivityAction;
        TStringBuf disrespectReply;
        TStringBuf reply;
        TStringBuf contextBuf = line;

        contextBuf.RSplit('\t', contextBuf, gifUrl);
        contextBuf.RSplit('\t', contextBuf, gifSourceText);
        contextBuf.RSplit('\t', contextBuf, gifSourceUrl);
        contextBuf.RSplit('\t', contextBuf, proactivityAction);
        contextBuf.RSplit('\t', contextBuf, source);
        contextBuf.RSplit('\t', contextBuf, disrespectReply);
        contextBuf.RSplit('\t', contextBuf, reply);

        TString context;
        for (auto c : contextBuf) {
            if (c == '\t') {
                context += " _EOS_ ";
            } else {
                context += c;
            }
        }

        TDocDescr desc;
        TBuffer blob;
        desc.UseBlob(&blob);
        TDocInfoExtWriter ext;
        ext.Add("context", context.data());
        ext.Add("reply", TString{reply}.data());
        ext.Add("disrespect_reply", TString{disrespectReply}.data());
        ext.Add("source", TString{source}.data());
        ext.Add("proactivity_action", TString{proactivityAction}.data());
        ext.Add("gif_source_url", TString{gifSourceUrl}.data());
        ext.Add("gif_source_text", TString{gifSourceText}.data());
        ext.Add("gif_url", TString{gifUrl}.data());
        ext.Write(desc);
        WriteEmptyDoc(output, blob.Data(), blob.Size(), nullptr, 0, docId++);
    }
    Cerr << docId << " documents were written" << Endl;

    // write indexarc/indexdir
    output.Finish();
    MakeArchiveDir(arcFile, dirFile);

    return 0;
}
