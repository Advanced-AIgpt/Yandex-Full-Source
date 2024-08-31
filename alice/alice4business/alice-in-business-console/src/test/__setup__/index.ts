import enzyme from 'enzyme';
import Adapter from 'enzyme-adapter-react-16';
import config from '../../server/lib/config';

enzyme.configure({ adapter: new Adapter() });

(global as any).enzyme = enzyme;
