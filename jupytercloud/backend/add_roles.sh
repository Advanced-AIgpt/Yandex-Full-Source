curl -X POST -d 'login=lipkin&path=/role/admin/&role={"role": "admin"}' http://localhost:8000/services/idm/add-role
echo ""
curl -X POST -d 'login=lipkin&path=/role/admin/&role={"role": "admin"}' http://localhost:8000/services/idm/remove-role
echo ""
curl -X POST -d 'login=lipkin&path=/role/admin/&role={"role": "admin"}' http://localhost:8000/services/idm/add-role
echo ""

curl -X POST -d 'login=lipkin&path=/role/quota/vm/cpu1_ram4_hdd24/&role={"role": "quota", "vm": "cpu1_ram4_hdd24"}&fields={"quota": "2"}' http://localhost:8000/services/idm/add-role
echo ""

curl -X POST -d 'login=lipkin&path=/role/quota/vm/cpu1_ram4_hdd24/&role={"role": "quota", "vm": "cpu1_ram4_hdd24"}&fields={"quota": "1"}' http://localhost:8000/services/idm/remove-role
echo ""

curl -X POST -d 'login=lipkin&path=/role/quota/vm/cpu1_ram4_hdd24/&role={"role": "quota", "vm": "cpu1_ram4_hdd24"}' http://localhost:8000/services/idm/add-role
echo ""
curl -X POST -d 'login=foo&path=/role/quota/vm/cpu1_ram4_hdd24/&role={"role": "quota", "vm": "cpu1_ram4_hdd24"}' http://localhost:8000/services/idm/add-role
echo ""

curl http://localhost:8000/services/idm/get-all-roles
echo ""

curl http://localhost:8000/services/idm/info
echo ""
