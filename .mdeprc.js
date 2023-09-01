exports.node = "18";
exports.auto_compose = true;
exports.services = ['redis'];
exports.nycCoverage = false;
exports.nycReport = false;
exports.test_framework = 'mocha';
exports.pre = 'rimraf ./coverage/tmp';
exports.post_exec = 'yarn c8 report -r text -r lcov';
exports.extras = {
  tester: {
    environment: {
      NODE_V8_COVERAGE: 'coverage/tmp'
    }
  }
}