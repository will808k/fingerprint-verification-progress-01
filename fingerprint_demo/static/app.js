async function api(path, opts = {}) {
  const res = await fetch(path, {
    headers: { "Content-Type": "application/json", ...(opts.headers || {}) },
    ...opts,
  });
  const data = await res.json().catch(() => ({ ok: false, error: "Invalid JSON response" }));
  if (!res.ok || data.ok === false) {
    const err = new Error(data.error || `HTTP ${res.status}`);
    err.data = data;
    throw err;
  }
  return data;
}

const $ = (id) => document.getElementById(id);

function setPill(opened, text) {
  const el = $("devicePill");
  el.textContent = text;
  el.classList.toggle("ok", !!opened);
  el.classList.toggle("bad", !opened);
}

function renderUsers(users) {
  const body = $("userRows");
  if (!users || !users.length) {
    body.innerHTML = `<tr><td colspan="5">No users yet</td></tr>`;
    return;
  }
  body.innerHTML = users
    .map(
      (u) => `<tr>
        <td><code>${escapeHtml(u.id)}</code></td>
        <td>${escapeHtml(u.name)}</td>
        <td>${u.template_bytes} B</td>
        <td>${escapeHtml(u.created_at || "")}</td>
        <td><button data-del="${escapeHtml(u.id)}">Delete</button></td>
      </tr>`
    )
    .join("");
  body.querySelectorAll("button[data-del]").forEach((btn) => {
    btn.addEventListener("click", async () => {
      if (!confirm(`Delete user ${btn.dataset.del}?`)) return;
      await api("/api/users/delete", {
        method: "POST",
        body: JSON.stringify({ user_id: btn.dataset.del }),
      });
      await refresh();
    });
  });
}

function escapeHtml(s) {
  return String(s)
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;");
}

function renderEnroll(enroll) {
  const count = enroll.count || 0;
  $("enrollBar").style.width = `${(count / 3) * 100}%`;
  const st = $("enrollStatus");
  st.textContent = enroll.error
    ? `${enroll.message} — ${enroll.error}`
    : enroll.message || (enroll.active ? "Working..." : "Idle");
  st.className = "status " + (enroll.error ? "error" : enroll.count === 3 && !enroll.active ? "ok" : "");

  const wrap = $("enrollPreviews");
  wrap.innerHTML = (enroll.previews || [])
    .map((b64) => (b64 ? `<img src="data:image/png;base64,${b64}" alt="capture" />` : ""))
    .join("");

  $("btnEnroll").disabled = !!enroll.active;
  $("btnEnrollCancel").disabled = !enroll.active;
}

async function refresh() {
  const data = await api("/api/status");
  setPill(
    data.opened,
    data.opened
      ? `Device open · SN ${data.serial || "?"} · ${data.width}×${data.height}`
      : `Device closed · detected ${data.device_count}`
  );
  $("deviceMeta").textContent = data.opened
    ? `Ready for enroll / verify`
    : `Click Open device (may need root / plugdev after setup_device.sh)`;
  renderUsers(data.users);
  renderEnroll(data.enroll || {});
}

$("btnOpen").onclick = async () => {
  try {
    await api("/api/open", { method: "POST", body: "{}" });
    await refresh();
  } catch (e) {
    alert(`Open failed: ${e.message}`);
  }
};

$("btnClose").onclick = async () => {
  await api("/api/close", { method: "POST", body: "{}" });
  await refresh();
};

$("btnRefresh").onclick = () => refresh();

$("btnEnroll").onclick = async () => {
  const user_id = $("enrollId").value.trim();
  const name = $("enrollName").value.trim();
  if (!user_id) {
    alert("Enter a User ID");
    return;
  }
  try {
    await api("/api/enroll/start", {
      method: "POST",
      body: JSON.stringify({ user_id, name }),
    });
    await refresh();
  } catch (e) {
    alert(`Enroll start failed: ${e.message}`);
  }
};

$("btnEnrollCancel").onclick = async () => {
  await api("/api/enroll/cancel", { method: "POST", body: "{}" });
  await refresh();
};

$("btnVerify").onclick = async () => {
  const st = $("verifyStatus");
  const box = $("verifyResult");
  const img = $("verifyPreview");
  st.textContent = "Place finger on the reader...";
  st.className = "status";
  box.textContent = "";
  img.classList.add("hidden");
  $("btnVerify").disabled = true;
  try {
    const data = await api("/api/verify", {
      method: "POST",
      body: JSON.stringify({
        user_id: $("verifyId").value.trim(),
        threshold: Number($("threshold").value || 45),
        timeout_ms: 12000,
      }),
    });
    if (data.preview_png_b64) {
      img.src = `data:image/png;base64,${data.preview_png_b64}`;
      img.classList.remove("hidden");
    }
    const best = data.best;
    if (data.matched && best) {
      st.textContent = `MATCH · ${best.name} (${best.id}) · score ${best.score}`;
      st.className = "status ok";
    } else {
      st.textContent = best
        ? `NO MATCH · best=${best.id} score=${best.score} (threshold ${data.threshold})`
        : "NO MATCH · empty database";
      st.className = "status error";
    }
    box.textContent = JSON.stringify({ matched: data.matched, best, top: data.results }, null, 2);
  } catch (e) {
    st.textContent = `Verify failed: ${e.message}`;
    st.className = "status error";
  } finally {
    $("btnVerify").disabled = false;
  }
};

$("btnClear").onclick = async () => {
  if (!confirm("Delete ALL enrolled templates?")) return;
  await api("/api/users/clear", { method: "POST", body: "{}" });
  await refresh();
};

// Poll while enrollment is active
setInterval(async () => {
  try {
    const e = await api("/api/enroll/status");
    renderEnroll(e);
    if (!e.active && e.count === 3) {
      // refresh user table after successful enroll
      const st = await api("/api/users");
      renderUsers(st.users);
    }
  } catch (_) {
    /* ignore transient errors while server is busy capturing */
  }
}, 800);

refresh().catch((e) => {
  setPill(false, "Server error");
  console.error(e);
});
