import os
import shutil
import tarfile
import tempfile

import vh
from dict.mt.make.libs.block import block
from dict.mt.make.libs.common import MTFS, MTFSUpdate
from dict.mt.make.libs.types import MTPath
from typing import Optional


@block
class ConvertTfnnToProdPipeline:
    input_bundle_path: MTPath
    output_file_path: MTPath

    def __call__(self, mtdata: MTFS) -> MTFSUpdate:
        input_file = mtdata[self.input_bundle_path]

        binary = vh.arcadia_executable('dict/mt/make/tools/tfnn/convert_tfnn_to_mtd', revision=6222979)

        output_file = vh.tgt(
            'prod_bundle',
            binary,
            input_file,
            recipe='mkdir tmp_dir && '
                   'tar xvf {{ input_file }} -C tmp_dir && '
                   '{{ binary }} '
                   'tmp_dir/model.npz '
                   'tmp_dir/model.npz '
                   '--arch transformer --max-rel-pos 0 && '
                   'tar cvf {{ OUT }} -C tmp_dir .',
            hardware_params=vh.HardwareParams(max_disk=50000, max_ram=4096)
        )

        return {
            self.output_file_path: output_file
        }


@vh.lazy.from_annotations
def training_checkpoint_to_bundle(
        input_file: vh.File, bpe_file: vh.File, token_to_id_file: vh.File,
        hp_file: vh.File, model_filename: vh.mkinput(str, nargs='?'), out_file: vh.mkoutput(vh.File)) -> vh.File:
    archive = tarfile.open(str(input_file))
    archive_members = archive.getmembers()

    if len(model_filename) > 0:
        model_filename = model_filename[0]

        the_checkpoint = None
        for member in archive_members:
            if model_filename == member.name[len('./model/checkpoint/'):]:
                the_checkpoint = member

        if the_checkpoint is None:
            raise ValueError('Specified filename `{}` is not in dir. All files: {}'.format(model_filename, archive_members))
    else:
        best_dev_checkpoint, final_checkpoint, latest_checkpoint = None, None, None
        step = -1

        for member in archive_members:
            name = member.name[len('./model/checkpoint/'):]
            if name == 'model-best_dev.npz':
                best_dev_checkpoint = member
                print('Found best dev checkpoint!')
                break
            elif name == 'model-final.npz':
                final_checkpoint = member
                break
            elif name.startswith('model-'):
                if 'latest' in name:
                    continue

                found_step = int(name[len('model-'):-len('.npz')])
                if step < found_step:
                    step = found_step
                    latest_checkpoint = member

        assert best_dev_checkpoint is not None or final_checkpoint is not None or latest_checkpoint is not None

        if best_dev_checkpoint is not None:
            the_checkpoint = best_dev_checkpoint
        elif final_checkpoint is not None:
            the_checkpoint = final_checkpoint
        else:
            the_checkpoint = latest_checkpoint

    new_folder = tempfile.mkdtemp()

    print('Found checkpoint: {}'.format(the_checkpoint.name))
    the_checkpoint.name = os.path.join(new_folder, 'model.npz')
    archive.extract(the_checkpoint)
    shutil.copy(str(bpe_file), os.path.join(new_folder, 'bpe.voc'))
    shutil.copy(str(token_to_id_file), os.path.join(new_folder, 'token_to_id.voc'))
    shutil.copy(str(hp_file), os.path.join(new_folder, 'hp.json'))

    temp_bundle = tempfile.mktemp()
    shutil.make_archive(temp_bundle, 'tar', new_folder)
    shutil.move(temp_bundle + '.tar', out_file)

    return out_file


@block
class ConvertTrainingCheckpointToBundle:
    bpe_path: MTPath
    token_to_id_path: MTPath
    hp_path: MTPath
    output_bundle_path: MTPath
    training_checkpoint_path: Optional[MTPath] = None
    training_checkpoint_url: Optional[str] = None

    def __call__(self, mtdata: MTFS) -> MTFSUpdate:
        assert (self.training_checkpoint_path is None) != (self.training_checkpoint_url is None)

        output_file = vh.File('tmp.ytfile')

        if self.training_checkpoint_path is not None:
            training_ckpt = mtdata[self.training_checkpoint_path]
        else:
            training_ckpt = vh.op(id='3f8a4b71-861c-4abf-a34d-8ba6b50ea882')(
                _options={'url': self.training_checkpoint_url},
                out_type='binary',
                max_disk=100000
            )

        with vh.HardwareParams(max_disk=100 * 1000, max_ram=16384, gpu_count=1, gpu_type=vh.GPUType.CUDA_7_0):  # 100GB

            training_checkpoint_to_bundle(
                training_ckpt,
                mtdata[self.bpe_path],
                mtdata[self.token_to_id_path],
                mtdata[self.hp_path],
                output_file
            )

        return {
            self.output_bundle_path: output_file
        }
