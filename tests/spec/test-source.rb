#!/usr/bin/env ruby
require_relative 'helper'

describe "debugger 'source' command" do
  include RemakeTestHelper
  it 'should be able to do run a source command' do
    opts = {
      :filter => Filter_filename, 
      :flags  => '-X  -f',
      :input  => "echo 'source #{DATA_DIR}/remakerc
quit
'"
    }
    test_name = File.basename(__FILE__, '.rb')[5..-1]
    run_remake(test_name, opts, 'triple')
  end
end
