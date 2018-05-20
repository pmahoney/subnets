module Summarize
  def test_summarizing_net_includes_subnets
    random = Random.new
    start = Time.now
    until Time.now - start > TIMED_TEST_DURATION
      nets = (1..3).map{klass.random(random)}
      summ = klass.summarize(nets)
      assert_operator summ.prefixlen, :>=, 0, "summarized #{nets} to invalid #{summ}"
      assert_summarizes klass.summarize(nets), nets
    end
  end
end
