from vins_core.nlu.sample_processors.registry import register_sample_processor
from navi_app.lib.address_fraction import AddressFractionSamplesProcessor

register_sample_processor(AddressFractionSamplesProcessor, 'address_fraction')