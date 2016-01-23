// Copyright 2014 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "iptables.h"

#include <gtest/gtest.h>

#include "mock_iptables.h"

namespace {
const char kIpTablesPath[] = "/sbin/iptables";
const char kIp6TablesPath[] = "/sbin/ip6tables";
}  // namespace

namespace firewalld {

using testing::_;
using testing::Return;

class IpTablesTest : public testing::Test {
 public:
  IpTablesTest() = default;
  ~IpTablesTest() override = default;

 protected:
  void SetMockExpectations(MockIpTables* iptables, bool success) {
    EXPECT_CALL(*iptables, AddAcceptRule(_, _, _, _))
        .WillRepeatedly(Return(success));
    EXPECT_CALL(*iptables, DeleteAcceptRule(_, _, _, _))
        .WillRepeatedly(Return(success));
  }

  void SetMockExpectationsPerExecutable(MockIpTables* iptables,
                                        bool ip4_success,
                                        bool ip6_success) {
    EXPECT_CALL(*iptables, AddAcceptRule(kIpTablesPath, _, _, _))
        .WillRepeatedly(Return(ip4_success));
    EXPECT_CALL(*iptables, AddAcceptRule(kIp6TablesPath, _, _, _))
        .WillRepeatedly(Return(ip6_success));
    EXPECT_CALL(*iptables, DeleteAcceptRule(kIpTablesPath, _, _, _))
        .WillRepeatedly(Return(ip4_success));
    EXPECT_CALL(*iptables, DeleteAcceptRule(kIp6TablesPath, _, _, _))
        .WillRepeatedly(Return(ip6_success));
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(IpTablesTest);
};

TEST_F(IpTablesTest, Port0Fails) {
  MockIpTables mock_iptables;
  // We should not be adding any rules for port 0.
  EXPECT_CALL(mock_iptables, AddAcceptRule(_, _, _, _)).Times(0);
  EXPECT_CALL(mock_iptables, DeleteAcceptRule(_, _, _, _)).Times(0);
  // Try to punch hole for TCP port 0, port 0 is not a valid port.
  EXPECT_FALSE(mock_iptables.PunchTcpHole(0, "iface"));
  // Try to punch hole for UDP port 0, port 0 is not a valid port.
  EXPECT_FALSE(mock_iptables.PunchUdpHole(0, "iface"));
}

TEST_F(IpTablesTest, ValidInterfaceName) {
  MockIpTables mock_iptables;
  SetMockExpectations(&mock_iptables, true /* success */);

  EXPECT_TRUE(mock_iptables.PunchTcpHole(80, "shortname"));
  EXPECT_TRUE(mock_iptables.PunchUdpHole(53, "shortname"));
  EXPECT_TRUE(mock_iptables.PunchTcpHole(80, "middle-dash"));
  EXPECT_TRUE(mock_iptables.PunchUdpHole(53, "middle-dash"));
  EXPECT_TRUE(mock_iptables.PunchTcpHole(80, "middle.dot"));
  EXPECT_TRUE(mock_iptables.PunchUdpHole(53, "middle.dot"));
}

TEST_F(IpTablesTest, InvalidInterfaceName) {
  MockIpTables mock_iptables;
  // We should not be adding any rules for invalid interface names.
  EXPECT_CALL(mock_iptables, AddAcceptRule(_, _, _, _)).Times(0);
  EXPECT_CALL(mock_iptables, DeleteAcceptRule(_, _, _, _)).Times(0);

  EXPECT_FALSE(mock_iptables.PunchTcpHole(80, "reallylonginterfacename"));
  EXPECT_FALSE(mock_iptables.PunchTcpHole(80, "with spaces"));
  EXPECT_FALSE(mock_iptables.PunchTcpHole(80, "with$ymbols"));
  EXPECT_FALSE(mock_iptables.PunchTcpHole(80, "-startdash"));
  EXPECT_FALSE(mock_iptables.PunchTcpHole(80, "enddash-"));
  EXPECT_FALSE(mock_iptables.PunchTcpHole(80, ".startdot"));
  EXPECT_FALSE(mock_iptables.PunchTcpHole(80, "enddot."));

  EXPECT_FALSE(mock_iptables.PunchUdpHole(53, "reallylonginterfacename"));
  EXPECT_FALSE(mock_iptables.PunchUdpHole(53, "with spaces"));
  EXPECT_FALSE(mock_iptables.PunchUdpHole(53, "with$ymbols"));
  EXPECT_FALSE(mock_iptables.PunchUdpHole(53, "-startdash"));
  EXPECT_FALSE(mock_iptables.PunchUdpHole(53, "enddash-"));
  EXPECT_FALSE(mock_iptables.PunchUdpHole(53, ".startdot"));
  EXPECT_FALSE(mock_iptables.PunchUdpHole(53, "enddot."));
}

TEST_F(IpTablesTest, PunchTcpHoleSucceeds) {
  MockIpTables mock_iptables;
  SetMockExpectations(&mock_iptables, true /* success */);

  // Punch hole for TCP port 80, should succeed.
  EXPECT_TRUE(mock_iptables.PunchTcpHole(80, "iface"));
  // Punch again, should still succeed.
  EXPECT_TRUE(mock_iptables.PunchTcpHole(80, "iface"));
  // Plug the hole, should succeed.
  EXPECT_TRUE(mock_iptables.PlugTcpHole(80, "iface"));
}

TEST_F(IpTablesTest, PlugTcpHoleSucceeds) {
  MockIpTables mock_iptables;
  SetMockExpectations(&mock_iptables, true /* success */);

  // Punch hole for TCP port 80, should succeed.
  EXPECT_TRUE(mock_iptables.PunchTcpHole(80, "iface"));
  // Plug the hole, should succeed.
  EXPECT_TRUE(mock_iptables.PlugTcpHole(80, "iface"));
  // Plug again, should fail.
  EXPECT_FALSE(mock_iptables.PlugTcpHole(80, "iface"));
}

TEST_F(IpTablesTest, PunchUdpHoleSucceeds) {
  MockIpTables mock_iptables;
  SetMockExpectations(&mock_iptables, true /* success */);

  // Punch hole for UDP port 53, should succeed.
  EXPECT_TRUE(mock_iptables.PunchUdpHole(53, "iface"));
  // Punch again, should still succeed.
  EXPECT_TRUE(mock_iptables.PunchUdpHole(53, "iface"));
  // Plug the hole, should succeed.
  EXPECT_TRUE(mock_iptables.PlugUdpHole(53, "iface"));
}

TEST_F(IpTablesTest, PlugUdpHoleSucceeds) {
  MockIpTables mock_iptables;
  SetMockExpectations(&mock_iptables, true /* success */);

  // Punch hole for UDP port 53, should succeed.
  EXPECT_TRUE(mock_iptables.PunchUdpHole(53, "iface"));
  // Plug the hole, should succeed.
  EXPECT_TRUE(mock_iptables.PlugUdpHole(53, "iface"));
  // Plug again, should fail.
  EXPECT_FALSE(mock_iptables.PlugUdpHole(53, "iface"));
}

TEST_F(IpTablesTest, PunchTcpHoleFails) {
  MockIpTables mock_iptables;
  SetMockExpectations(&mock_iptables, false /* success */);
  // Punch hole for TCP port 80, should fail.
  ASSERT_FALSE(mock_iptables.PunchTcpHole(80, "iface"));
}

TEST_F(IpTablesTest, PunchUdpHoleFails) {
  MockIpTables mock_iptables;
  SetMockExpectations(&mock_iptables, false /* success */);
  // Punch hole for UDP port 53, should fail.
  ASSERT_FALSE(mock_iptables.PunchUdpHole(53, "iface"));
}

TEST_F(IpTablesTest, PunchTcpHoleIpv6Fails) {
  MockIpTables mock_iptables;
  SetMockExpectationsPerExecutable(&mock_iptables, true /* ip4_success */,
                                   false /* ip6_success */);
  // Punch hole for TCP port 80, should fail because 'ip6tables' fails.
  ASSERT_FALSE(mock_iptables.PunchTcpHole(80, "iface"));
}

TEST_F(IpTablesTest, PunchUdpHoleIpv6Fails) {
  MockIpTables mock_iptables;
  SetMockExpectationsPerExecutable(&mock_iptables, true /* ip4_success */,
                                   false /* ip6_success */);
  // Punch hole for UDP port 53, should fail because 'ip6tables' fails.
  ASSERT_FALSE(mock_iptables.PunchUdpHole(53, "iface"));
}

TEST_F(IpTablesTest, ApplyVpnSetupAddSuccess) {
  const std::vector<std::string> usernames = {"testuser0", "testuser1"};
  const std::string interface = "ifc0";
  const bool add = true;

  MockIpTables mock_iptables;
  EXPECT_CALL(mock_iptables, ApplyMasquerade(kIpTablesPath, interface, add))
      .WillOnce(Return(true));
  EXPECT_CALL(mock_iptables, ApplyMasquerade(kIp6TablesPath, interface, add))
      .WillOnce(Return(true));
  EXPECT_CALL(mock_iptables,
              ApplyMarkForUserTraffic(kIpTablesPath, usernames[0], add))
      .WillOnce(Return(true));
  EXPECT_CALL(mock_iptables,
              ApplyMarkForUserTraffic(kIp6TablesPath, usernames[0], add))
      .WillOnce(Return(true));
  EXPECT_CALL(mock_iptables,
              ApplyMarkForUserTraffic(kIpTablesPath, usernames[1], add))
      .WillOnce(Return(true));
  EXPECT_CALL(mock_iptables,
              ApplyMarkForUserTraffic(kIp6TablesPath, usernames[1], add))
      .WillOnce(Return(true));
  EXPECT_CALL(mock_iptables, ApplyRuleForUserTraffic(kIPv4, add))
      .WillOnce(Return(true));
  EXPECT_CALL(mock_iptables, ApplyRuleForUserTraffic(kIPv6, add))
      .WillOnce(Return(true));

  ASSERT_TRUE(
      mock_iptables.ApplyVpnSetup(usernames, interface, add));
}

TEST_F(IpTablesTest, ApplyVpnSetupAddFailureInUsername) {
  const std::vector<std::string> usernames = {"testuser0", "testuser1"};
  const std::string interface = "ifc0";
  const bool remove = false;
  const bool add = true;

  MockIpTables mock_iptables;
  EXPECT_CALL(mock_iptables, ApplyMasquerade(kIpTablesPath, interface, add))
      .Times(1)
      .WillOnce(Return(true));
  EXPECT_CALL(mock_iptables, ApplyMasquerade(kIp6TablesPath, interface, add))
      .Times(1)
      .WillOnce(Return(true));
  EXPECT_CALL(mock_iptables,
              ApplyMarkForUserTraffic(kIpTablesPath, usernames[0], add))
      .Times(1)
      .WillOnce(Return(true));
  EXPECT_CALL(mock_iptables,
              ApplyMarkForUserTraffic(kIp6TablesPath, usernames[0], add))
      .Times(1)
      .WillOnce(Return(true));
  EXPECT_CALL(mock_iptables,
              ApplyMarkForUserTraffic(kIpTablesPath, usernames[1], add))
      .Times(1)
      .WillOnce(Return(false));
  EXPECT_CALL(mock_iptables, ApplyRuleForUserTraffic(kIPv4, add))
      .Times(1)
      .WillOnce(Return(true));
  EXPECT_CALL(mock_iptables, ApplyRuleForUserTraffic(kIPv6, add))
      .Times(1)
      .WillOnce(Return(true));

  EXPECT_CALL(mock_iptables, ApplyMasquerade(kIpTablesPath, interface, remove))
      .Times(1)
      .WillOnce(Return(true));
  EXPECT_CALL(mock_iptables, ApplyMasquerade(kIp6TablesPath, interface, remove))
      .Times(1)
      .WillOnce(Return(true));
  EXPECT_CALL(mock_iptables,
              ApplyMarkForUserTraffic(kIpTablesPath, usernames[0], remove))
      .Times(1)
      .WillOnce(Return(false));
  EXPECT_CALL(mock_iptables,
              ApplyMarkForUserTraffic(kIp6TablesPath, usernames[0], remove))
      .Times(1)
      .WillOnce(Return(false));
  EXPECT_CALL(mock_iptables,
              ApplyMarkForUserTraffic(kIpTablesPath, usernames[1], remove))
              .Times(0);
  EXPECT_CALL(mock_iptables, ApplyRuleForUserTraffic(kIPv4, remove))
      .Times(1)
      .WillOnce(Return(false));
  EXPECT_CALL(mock_iptables, ApplyRuleForUserTraffic(kIPv6, remove))
      .Times(1)
      .WillOnce(Return(false));

  ASSERT_FALSE(
      mock_iptables.ApplyVpnSetup(usernames, interface, add));
}

TEST_F(IpTablesTest, ApplyVpnSetupAddFailureInMasquerade) {
  const std::vector<std::string> usernames = {"testuser0", "testuser1"};
  const std::string interface = "ifc0";
  const bool remove = false;
  const bool add = true;

  MockIpTables mock_iptables;
  EXPECT_CALL(mock_iptables, ApplyMasquerade(kIpTablesPath, interface, add))
      .Times(1)
      .WillOnce(Return(false));
  EXPECT_CALL(mock_iptables, ApplyMarkForUserTraffic(_, _, _)).Times(0);
  EXPECT_CALL(mock_iptables, ApplyRuleForUserTraffic(kIPv4, add))
      .Times(1)
      .WillOnce(Return(true));
  EXPECT_CALL(mock_iptables, ApplyRuleForUserTraffic(kIPv6, add))
      .Times(1)
      .WillOnce(Return(true));

  EXPECT_CALL(mock_iptables, ApplyMasquerade(kIpTablesPath, interface, remove))
      .Times(1)
      .WillOnce(Return(true));
  EXPECT_CALL(mock_iptables, ApplyMasquerade(kIp6TablesPath, interface, remove))
      .Times(1)
      .WillOnce(Return(true));
  EXPECT_CALL(mock_iptables, ApplyRuleForUserTraffic(kIPv4, remove))
      .Times(1)
      .WillOnce(Return(true));
  EXPECT_CALL(mock_iptables, ApplyRuleForUserTraffic(kIPv6, remove))
      .Times(1)
      .WillOnce(Return(true));

  ASSERT_FALSE(
      mock_iptables.ApplyVpnSetup(usernames, interface, add));
}

TEST_F(IpTablesTest, ApplyVpnSetupAddFailureInRuleForUserTraffic) {
  const std::vector<std::string> usernames = {"testuser0", "testuser1"};
  const std::string interface = "ifc0";
  const bool remove = false;
  const bool add = true;

  MockIpTables mock_iptables;
  EXPECT_CALL(mock_iptables, ApplyMasquerade(kIpTablesPath, interface, _))
      .Times(0);
  EXPECT_CALL(mock_iptables, ApplyMarkForUserTraffic(_, _, _)).Times(0);
  EXPECT_CALL(mock_iptables, ApplyRuleForUserTraffic(kIPv4, add))
      .Times(1)
      .WillOnce(Return(false));

  EXPECT_CALL(mock_iptables, ApplyRuleForUserTraffic(kIPv4, remove)).Times(0);

  ASSERT_FALSE(
      mock_iptables.ApplyVpnSetup(usernames, interface, add));
}

TEST_F(IpTablesTest, ApplyVpnSetupRemoveSuccess) {
  const std::vector<std::string> usernames = {"testuser0", "testuser1"};
  const std::string interface = "ifc0";
  const bool remove = false;
  const bool add = true;

  MockIpTables mock_iptables;
  EXPECT_CALL(mock_iptables, ApplyMasquerade(kIpTablesPath, interface, remove))
      .Times(1)
      .WillOnce(Return(true));
  EXPECT_CALL(mock_iptables, ApplyMasquerade(kIp6TablesPath, interface, remove))
      .Times(1)
      .WillOnce(Return(true));
  EXPECT_CALL(mock_iptables, ApplyMarkForUserTraffic(_, _, remove))
      .Times(4)
      .WillRepeatedly(Return(true));
  EXPECT_CALL(mock_iptables, ApplyRuleForUserTraffic(kIPv4, remove))
      .Times(1)
      .WillOnce(Return(true));
  EXPECT_CALL(mock_iptables, ApplyRuleForUserTraffic(kIPv6, remove))
      .Times(1)
      .WillOnce(Return(true));

  EXPECT_CALL(mock_iptables, ApplyMasquerade(kIpTablesPath, interface, add))
      .Times(0);
  EXPECT_CALL(mock_iptables, ApplyMarkForUserTraffic(_, _, add))
      .Times(0);
  EXPECT_CALL(mock_iptables, ApplyRuleForUserTraffic(kIPv4, add)).Times(0);

  ASSERT_TRUE(
      mock_iptables.ApplyVpnSetup(usernames, interface, remove));
}

TEST_F(IpTablesTest, ApplyVpnSetupRemoveFailure) {
  const std::vector<std::string> usernames = {"testuser0", "testuser1"};
  const std::string interface = "ifc0";
  const bool remove = false;
  const bool add = true;

  MockIpTables mock_iptables;
  EXPECT_CALL(mock_iptables, ApplyMasquerade(kIpTablesPath, interface, remove))
      .Times(1)
      .WillRepeatedly(Return(false));
  EXPECT_CALL(mock_iptables, ApplyMasquerade(kIp6TablesPath, interface, remove))
      .Times(1)
      .WillRepeatedly(Return(false));
  EXPECT_CALL(mock_iptables, ApplyMarkForUserTraffic(_, _, remove))
      .Times(4)
      .WillRepeatedly(Return(false));
  EXPECT_CALL(mock_iptables, ApplyRuleForUserTraffic(kIPv4, remove))
      .Times(1)
      .WillRepeatedly(Return(false));
  EXPECT_CALL(mock_iptables, ApplyRuleForUserTraffic(kIPv6, remove))
      .Times(1)
      .WillRepeatedly(Return(false));

  EXPECT_CALL(mock_iptables, ApplyMasquerade(kIpTablesPath, interface, add))
      .Times(0);
  EXPECT_CALL(mock_iptables, ApplyMarkForUserTraffic(_, _, add))
      .Times(0);
  EXPECT_CALL(mock_iptables, ApplyRuleForUserTraffic(kIPv4, add)).Times(0);

  ASSERT_FALSE(
      mock_iptables.ApplyVpnSetup(usernames, interface, remove));
}

}  // namespace firewalld
