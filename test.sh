#! /usr/bin/env bash

set -eu

rm -rf ./reports

for RUBY in ruby_2_3 ruby ruby_2_5; do
  printf "=== Running tests with nixpkgs.%s\\n" "$RUBY"
  nix-shell --argstr rubypkg "$RUBY" --run "
    ruby --version && bundle version && bundle install --quiet &&
    bundle exec rake compile && bundle exec rake test"

  mkdir -p reports/"$RUBY"
  mv test/reports/* reports/"$RUBY"
done
