export async function run({ page, expect }) {
  const results = page.locator("#results");

  // Wait for mount() to run and populate results.
  await page.waitForFunction(() => document.querySelector("#results")?.getAttribute("data-ready") === "1");

  async function attr(name) {
    const v = await results.getAttribute(name);
    expect.ok(v !== null, `Expected ${name} to be present`);
    return v;
  }

  expect.ok((await attr("data-root-valid")) === "1");
  expect.ok((await attr("data-missing-valid")) === "0");
  expect.ok((await attr("data-contains-ok")) === "1");
  expect.ok((await attr("data-contains-not-ok")) === "0");
  expect.ok((await attr("data-closest-ok")) === "1");
  expect.ok((await attr("data-parent-valid")) === "1");
  expect.ok((await attr("data-parent-contains-root")) === "1");
  expect.ok((await attr("data-connected-ok")) === "1");

  expect.ok((await attr("data-qsa-count")) === "2");
  expect.ok((await attr("data-qsa-first-idx")) === "0");

  expect.ok((await attr("data-child-count")) === "2");
  expect.ok((await attr("data-child-first-id")) === "");

  expect.ok((await attr("data-active-valid")) === "1");
}
