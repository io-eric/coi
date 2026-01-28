export async function run({ page, expect }) {
  const status = page.locator(".status");
  await expect.textContains(status, "dom query test");

  // Wait for mount() to run and populate results.
  await page.waitForFunction(() => {
    const t = document.querySelector(".status")?.textContent ?? "";
    return t.includes("rootValid=1") && t.includes("missingValid=0");
  });

  await expect.textContains(status, "rootValid=1");
  await expect.textContains(status, "missingValid=0");
  await expect.textContains(status, "containsOk=1");
  await expect.textContains(status, "containsNotOk=0");
  await expect.textContains(status, "closestOk=1");
  await expect.textContains(status, "parentValid=1");
  await expect.textContains(status, "parentContainsRoot=1");
  await expect.textContains(status, "connectedOk=1");

  await expect.textContains(status, "qsaCount=2");
  await expect.textContains(status, "qsaFirstIdx=0");

  await expect.textContains(status, "childCount=2");
  await expect.textContains(status, "childFirstId=");

  await expect.textContains(status, "activeValid=1");
}
