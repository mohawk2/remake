#!/usr/bin/env ruby
require_relative 'helper'

describe "no readline" do
  include RemakeTestHelper
  it 'should be able to do work without readline support' do
    opts = {
      :filter     => Filter_filename, 
      :exitstatus => 0,
      :flags      => '-X --no-readline -f',
      :input      => "echo 'continue third
finish 2
'"
    }
    test_name = File.basename(__FILE__, '.rb')[5..-1]
    run_remake(test_name, opts, 'triple')
  end
end
