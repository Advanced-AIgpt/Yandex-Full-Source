var viewerApp = angular.module('viewerApp', ['ui.bootstrap']);

viewerApp.controller('ViewerController', function ViewerController($scope, $http, $interval) {
    $scope.status = 'empty';

    fillScopeWithData = function(data) {
        update_date = new Date(data['last_updated']);
        if (!$scope.last_updated || update_date.getTime() != $scope.last_updated.getTime()) {
            $scope.last_updated = update_date;
            $scope.tests = data['tests'];
            $scope.status = 'updated';
        }
    }

    hasDataFromFile = typeof(data) != 'undefined';

    updateTestStatus = function() {
        if (hasDataFromFile) {
            fillScopeWithData(data);
            return;
        }

        url = 'https://vins-bucket.s3.mdst.yandex.net/stable-integration-tests.json?rnd=' + new Date().getTime();  // To disable caching
        $http.get(url).then(
            function success(response) {
                fillScopeWithData(response.data);
            },
            function error(response) {
                $scope.last_updated = new Date();
                $scope.status = 'update_error';
            }
        );
    }

    if (!hasDataFromFile) {
        updateTestStatusTimer = $interval(
            updateTestStatus,
            10 * 1000  // Every 10 seconds
        );

        $scope.$on('$destroy', function() {
            $interval.cancel(updateTestStatusTimer);
        });
    }

    // Update test status for the first time
    updateTestStatus();
});