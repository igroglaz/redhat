var loginMaxLength = 15;
var passwordLength = 10;
var createLoginSubj = 'create-a2-login';
var resetPswdSubj = 'reset-a2-password';
var lookbackTimeMillis = minutes(15);

var dbServer = 'mysql.server.com:3306';
var dbUser = 'my-sb-user';
var dbPassword = 'my-db-password';
var dbName = 'my-db-name';

var tblLogins = 'logins';
var fldLogin = 'login';
var fldPassword = 'password';
var fldEmail = 'email';

var createLoginSuccess = GmailApp.createLabel('a2-create-login-success');
var createLoginFailed = GmailApp.createLabel('a2-create-login-failed');
var resetPasswordSuccess = GmailApp.createLabel('a2-create-login-success');
var resetPasswordFailed = GmailApp.createLabel('a2-create-login-failed');
var allowedCommands = [createLoginSubj, resetPswdSubj];

function checkNewEmail() {
  var batchSize = 5;
  for (var batch = 0; batch < 50; batch++) {
    var threads = GmailApp.getInboxThreads(batch * batchSize, batchSize);
    Logger.log("start batch " + batch);
    for (var i = 0; i < threads.length; i++) {
      var t = threads[i];
      var subj = t.getFirstMessageSubject().toLowerCase();
      var msgs = t.getMessages();
      var lastMsg = msgs[msgs.length-1];
      var old = Date.now() - lastMsg.getDate().getTime();
      if (old > lookbackTimeMillis) {
        return;
      }
      var msg = msgs[0];
      Logger.log(subj + ' : ' + msg.getFrom());
      if (msg.isUnread()) {
        // Logger.log(subj + ' : ' + msg.getFrom());
        if (allowedCommands.includes(subj)) {
          var email = msg.getFrom().replace(/^.+<([^>]+)>$/, "$1");
          var emailDomain = email.substring(email.indexOf('@'));
          if (emailDomain == '@gmail.com') {
            switch (subj.toLowerCase()) {
              case createLoginSubj:
                var login = translate(email).substring(0, loginMaxLength);
                var pswd = createPassword(msg);
                try {
                  createLogin(login, pswd, email);
                  msg.reply(`Your login has been created.\nlogin:"${login}" (first ${loginMaxLength} letters and digits of your email account),\npassword: "${pswd}"`);
                  createLoginSuccess.addToThread(t);
                } catch (err) {
                  Logger.log(err);
                  msg.reply('Error happened');
                  createLoginFailed.addToThread(t);
                }
                break;
              case resetPswdSubj:
                var pswd = createPassword(msg);
                try {
                  var login = resetPassword(email, pswd);
                  msg.reply(`Your password has been reset.\nlogin:"${login}", \npassword: "${pswd}"`);
                  resetPasswordSuccess.addToThread(t);
                } catch (err) {
                  Logger.log(err);
                  msg.reply('Error happened\n' + err);
                  resetPasswordFailed.addToThread(t);
                }
                break;
              default:
                return;
            }
          } else {
            msg.reply('Only gmail accounts are allowed.');
          }
          msg.markRead();
        }
      }
    }
  }
}

function createConnection() {
  var dbUrl = `jdbc:mysql://${dbServer}/${dbName}`;
  return Jdbc.getConnection(dbUrl, dbUser, dbPassword);
}

function createLogin(login, pswd, email) {
  var conn;
  try {
    conn = createConnection();
    var stmt = conn.prepareStatement(`INSERT INTO ${tblLogins} ( ${fldLogin}, ${fldPassword}, ${fldEmail}) values (?, ?, ?)`);
    stmt.setString(1, login);
    stmt.setString(2, sha1(pswd));
    stmt.setString(3, email);
    var res = stmt.executeUpdate();
    Logger.log(`user ${login} created. res = ${res}`);
  } finally {
    if (conn) {
      conn.close();
    }
  }
}

function resetPassword(email, pswd) {
  var conn;
  try {
    conn = createConnection();
    var stmt = conn.prepareStatement(`UPDATE ${tblLogins} SET ${fldPassword} = ? WHERE ${fldEmail} = ?`);
    stmt.setString(1, sha1(pswd));
    stmt.setString(2, email);
    stmt.executeUpdate();
    var res = stmt.executeUpdate();
    Logger.log(`password ${pswd} for user ${email} has been reset. res = ${res}`);
    var select = conn.prepareStatement(`SELECT ${fldLogin} FROM ${tblLogins} WHERE ${fldEmail} = ?`)
    select.setString(1, email);
    var resultSet = select.executeQuery();
    if (resultSet.first()) {
      return resultSet.getString(1);
    } else {
      Logger.log("resultSet.first() == false")
      throw "login not found";
    }
  } finally {
    if (conn) {
      conn.close();
    }
  }
}

function randomStr(m) {
  var m = m || 15; s = '', r = 'abcdefghijklmnopqrstuvwxyz0123456789';
  for (var i = 0; i < m; i++) { s += r.charAt(Math.floor(Math.random() * r.length)); }
  return s;
}

function translate(email) {
  var login = email.toLowerCase().replace('@gmail.com', '')
    .replace(/[^a-z0-9]/g, '');

  return login;
}

function minutes(mins) {
  return mins * 60000;
}

function hours(hrs) {
  return minutes(hrs * 60);
}

function createPassword(msg) {
  var txt = msg.getPlainBody();
  if (txt.length > 0) {
    var pswd = (txt.toLowerCase()
      .replace(/[^a-z0-9]/g, '')
      + randomStr(passwordLength))
      .substring(0, passwordLength)
    return pswd;
  } else {
    return randomStr(passwordLength);
  }
}

function sha1(str) {
  var hashData = Utilities.computeDigest(Utilities.DigestAlgorithm.SHA_1, str);
  return hashData.map(n => n < 0 ? n + 256 : n)
    .map(n => n.toString(16))
    .join("")
}
