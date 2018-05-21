module EqlAndHash
  def new_obj(c=klass)
    c.new(*constructor_args)
  end

  def other_obj(c=klass)
    c.new(*other_constructor_args)
  end

  def new_obj_subclass
    new_obj(Class.new(klass))
  end

  def test_eq
    a, b = [new_obj, new_obj]
    refute_equal a.object_id, b.object_id
    assert_equal a, b
  end

  def test_eq_subclass
    refute_equal new_obj, new_obj_subclass
  end

  def test_eq_string
    refute_equal new_obj, other_obj
    refute_equal new_obj, 'str'
  end

  def test_hash
    a, b, = [new_obj, new_obj]
    refute_equal a.object_id, b.object_id
    assert_equal a.hash, b.hash

    other = other_obj
    refute_equal a.hash, other.hash

    h = {}
    h[a] = 'found'
    assert_equal 'found', h[a]
    assert_equal 'found', h[b]
    assert_nil h[new_obj_subclass]
  end
end
