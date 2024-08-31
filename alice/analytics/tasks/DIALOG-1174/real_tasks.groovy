// Три скрипта здесь https://nirvana.yandex-team.ru/process/559b46ef-f988-4b72-b53d-71efde14985d/graph/FlowchartBlockOperation/6481f3b1-b758-4021-95b9-fca83238c3e3
// И здесь https://nirvana.yandex-team.ru/process/7764d55a-cff1-4bff-abce-b35b238c859d/graph/FlowchartBlockOperation/eb74cb5a-e5a4-437b-80b3-23a131f763c3

// А в регулярке eval_ нужны  для валидации https://nirvana.yandex-team.ru/flow/0a30ec8d-ffd4-4ad3-a315-05491a2c8485/ddea6156-6cf9-4874-96e7-4931c65012d2/graph/FlowchartBlockOperation/bafe2955-a989-4a28-907e-e26b75b2c330
// А на втором этапе будто бы и не нужны: https://nirvana.yandex-team.ru/flow/0a30ec8d-ffd4-4ad3-a315-05491a2c8485/6fe24e83-17ba-4841-b042-5137ada4d067/graph/FlowchartBlockOperation/c4ab2937-e896-4d73-9171-125bba1c04b9

in0.each {
  if (it.knownSolutions == null &&
      !it.inputValues.key.startsWith('eval_')) {
    out.write(["inputValues" : it.inputValues,
               "outputValues": it.outputValues,
               "submitTs": String.valueOf(new Date().getTime()),
               "algorithm": "DS",
               "probability":it.probability
              ])
  }
}
