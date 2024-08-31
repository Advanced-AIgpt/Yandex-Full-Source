from library.python.monitoring.solo.objects.solomon.sensor import Sensor

get_all_objects_errors = Sensor(
    project='memento',
    cluster='production',
    service='main_monitoring',
    sensor='http.in.requests_failure_rate',
    path='get_all_objects',
    host='cluster'
)
